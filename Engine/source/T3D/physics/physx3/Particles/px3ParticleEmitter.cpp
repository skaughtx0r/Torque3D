//-----------------------------------------------------------------------------
// Authors: 
//		Andrew MacIntyre - Aldyre Studios - aldyre.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/physics/physx3/Particles/Px3ParticleEmitter.h"
#include "T3D/physics/physx3/Particles/px3ParticleSystem.h"
#include "T3D/physics/physx3/px3.h"
#include "T3D/physics/physx3/px3World.h"
#include "T3D/physics/physx3/px3Plugin.h"
#include "T3D/physics/physx3/px3Cast.h"
#include "T3D/physics/physx3/px3Stream.h"

#include "console/consoleTypes.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mathIO.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "T3D/fx/particleEmitter.h"

IMPLEMENT_CO_NETOBJECT_V1( Px3ParticleEmitter );

ConsoleDocClass( Px3ParticleEmitter,
   
   "@brief Emitter for PhysX Particles\n\n"   
   "@ingroup Physics"
);

Px3ParticleEmitter::Px3ParticleEmitter()
  : mMatInst( NULL )
{
    mParticleSystem = NULL;

    mNetFlags.set( Ghostable | ScopeAlways );
    mTypeMask |= StaticObjectType | StaticShapeObjectType;

	emitterTick = 0;
	emitterRemoveTick = 0;
}

Px3ParticleEmitter::~Px3ParticleEmitter()
{
}

bool Px3ParticleEmitter::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Cloth is only created on the client.
   if ( isClientObject() )
   {
      mResetXfm = getTransform();
      PhysicsPlugin::getPhysicsResetSignal().notify( this, &Px3ParticleEmitter::onPhysicsReset, 1053.0f );

	  mParticleSystem = new Px3ParticleSystem();
	  mParticleSystem->create(100);
   }

   // On the server we use the static update
   // to setup the bounds of the cloth.
   if ( isServerObject() )
      _updateStaticSystem();

   addToScene();
   
   // Also the server object never ticks.
   if ( isServerObject() )
      setProcessTick( false );

   return true;
}

void Px3ParticleEmitter::onRemove()
{   
    SAFE_DELETE( mMatInst );
    if ( isClientObject() )
    {
        if ( mParticleSystem != NULL )
			mParticleSystem->release();

        PhysicsPlugin::getPhysicsResetSignal().remove( this, &Px3ParticleEmitter::onPhysicsReset );
    }

    removeFromScene();
    Parent::onRemove();
}

void Px3ParticleEmitter::onPhysicsReset( PhysicsResetEvent reset )
{
   // Store the reset transform for later use.
   if ( reset == PhysicsResetEvent_Store )
      mResetXfm = getTransform();

   // Recreate the cloth at the last reset position.
   _recreateSystem( mResetXfm );
}

void Px3ParticleEmitter::initPersistFields()
{
    Parent::initPersistFields();

    addField( "material", TypeMaterialName, Offset( mMaterialName, Px3ParticleEmitter ),
        "@brief Name of the material to render.\n\n" );

   // Cloth doesn't support scale.
   removeField( "scale" );
}

void Px3ParticleEmitter::inspectPostApply()
{
   Parent::inspectPostApply();
   //
   // if ( isServerObject() )
   //    _updateStaticSystem();

   setMaskBits( TransformMask | MaterialMask | ClothMask );
}

U32 Px3ParticleEmitter::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   if ( stream->writeFlag( mask & TransformMask ) )
      mathWrite( *stream, getTransform() );

   if ( stream->writeFlag( mask & MaterialMask ) )
      stream->write( mMaterialName );

   if ( stream->writeFlag( mask & ClothMask ) )
   {
        //stream->writeFlag( mCollisionEnabled );
        //stream->writeFlag( mWindEnabled );
   }

   return retMask;
}

void Px3ParticleEmitter::unpackUpdate( NetConnection *conn, BitStream *stream )
{
    Parent::unpackUpdate( conn, stream );

    // TransformMask
    if ( stream->readFlag() )
    {
        MatrixF mat;
        mathRead( *stream, &mat );
        setTransform( mat );
    }

    // MaterialMask
    if ( stream->readFlag() )
    {
        stream->read( &mMaterialName );
        SAFE_DELETE( mMatInst );
    }

    // ParticleMask
    if ( stream->readFlag() )
    {
        //mCollisionEnabled = stream->readFlag();
       // mWindEnabled = stream->readFlag();

        if (  isClientObject() && isProperlyAdded() &&
            !mParticleSystem )
        {
            //_createSystem();
        }

        _updateProperties();
    }
}

void Px3ParticleEmitter::_recreateSystem( const MatrixF &transform )
{
    if ( !mParticleSystem )
    {
        _updateStaticSystem();
        return;
    }

    Parent::setTransform( transform );
}

void Px3ParticleEmitter::setTransform( const MatrixF &mat )
{
   Parent::setTransform( mat );
   setMaskBits( TransformMask );

   // Only need to do this if we're on the server
   // or if we're not currently ticking physics.
   if ( !mParticleSystem )
      _updateStaticSystem();
}

void Px3ParticleEmitter::setScale( const VectorF &scale )
{
   // Cloth doesn't support scale as it has plenty
   // of complications... sharing meshes, thickness,
   // transform origin, etc.
   return;
}

void Px3ParticleEmitter::prepRenderImage( SceneRenderState *state )
{  
   /*if ( mIsVBDirty )
      _updateVBIB();

   // Recreate the material if we need to.
   if ( !mMatInst )
      _initMaterial();

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   BaseMatInstance *matInst = state->getOverrideMaterial( mMatInst );
   if ( !matInst )
      return;

   MeshRenderInst *ri = state->getRenderPass()->allocInst<MeshRenderInst>();

    // If we need lights then set them up.
   if ( matInst->isForwardLit() )
   {
      LightQuery query;
      query.init( getWorldSphere() );
        query.getLights( ri->lights, 8 );
   }

   ri->projection = state->getRenderPass()->allocSharedXform(RenderPassManager::Projection);
   ri->objectToWorld = &MatrixF::Identity;

   ri->worldToCamera = state->getRenderPass()->allocSharedXform(RenderPassManager::View);   
   ri->type = RenderPassManager::RIT_Mesh;

   ri->primBuff = &mPrimBuffer;
   ri->vertBuff = &mVB;

   ri->matInst = matInst;
   ri->prim = state->getRenderPass()->allocPrim();
   ri->prim->type = GFXTriangleList;
   ri->prim->minIndex = 0;
   ri->prim->startIndex = 0;
   ri->prim->numPrimitives = mNumIndices / 3;

   ri->prim->startVertex = 0;
   ri->prim->numVertices = mNumVertices;

   ri->defaultKey = matInst->getStateHint();
   ri->defaultKey2 = (U32)ri->vertBuff;

   state->getRenderPass()->addInst( ri );*/
}

void Px3ParticleEmitter::_updateProperties()
{
    if ( !mParticleSystem )
        return;
}

void Px3ParticleEmitter::_initMaterial()
{
    SAFE_DELETE( mMatInst );

    Material *material = NULL;
    if (mMaterialName.isNotEmpty() )
        Sim::findObject( mMaterialName, material );

    if ( material )
        mMatInst = material->createMatInstance();
    else
        mMatInst = MATMGR->createMatInstance( "WarningMaterial" );

    GFXStateBlockDesc desc;
    desc.setCullMode( GFXCullNone );
    mMatInst->addStateBlockDesc( desc );

    mMatInst->init( MATMGR->getDefaultFeatures(), getGFXVertexFormat<GFXVertexPNTT>() );
}

void Px3ParticleEmitter::_updateVBIB()
{
    if ( !mParticleSystem )
        return;

    /*PROFILE_SCOPE( Px3ParticleEmitter_UpdateVBIB );

    mIsVBDirty = false;

    // Don't set the VB if the vertex count is the same!
    if ( mVB.isNull() || mVB->mNumVerts < mNumVertices )
        mVB.set( GFX, mNumVertices, GFXBufferTypeDynamic );

    GFXVertexPNTT *vpPtr = mVB.lock();
    dMemcpy( vpPtr, mVertexRenderBuffer, sizeof( GFXVertexPNTT ) * mNumVertices );
    mVB.unlock();

    if ( mPrimBuffer.isNull() || mPrimBuffer->mIndexCount < mNumIndices )
        mPrimBuffer.set( GFX, mNumIndices, 0, GFXBufferTypeDynamic );

    U16 *pbPtr;
    mPrimBuffer.lock( &pbPtr );
    dMemcpy( pbPtr, mIndexRenderBuffer, sizeof( U16 ) * mNumIndices );
    mPrimBuffer.unlock();
	*/
}

void Px3ParticleEmitter::_updateStaticSystem()
{
    mObjBox.set( 0, 5, 0, 5, 5, 5 );
    resetWorldBox();
}

void Px3ParticleEmitter::processTick( const Move *move )
{
    // Make sure particle system exists
    if ( !mParticleSystem )
        return;

	// These are temp arbitrary values.
	mObjBox.set( 0, 5, 0, 5, 5, 5 );
    resetWorldBox();

	emitterTick++;
	emitterRemoveTick++;
	if ( emitterTick > 32 )
	{
		// Add a new particle every second.
		// mParticleSystem->addParticle(PxVec3(0, 0, 5), PxVec3(0, 1, 5));

		// Add 3 new particles every second.
		Vector<Point3F> newPos;
		Vector<Point3F> newVel;
		newPos.push_back(Point3F(1, 3, 5));
		newPos.push_back(Point3F(2, 2, 5));
		newPos.push_back(Point3F(3, 1, 5));
		newVel.push_back(Point3F(1, 3, 5));
		newVel.push_back(Point3F(2, 2, 5));
		newVel.push_back(Point3F(3, 1, 5));
		U32 count = mParticleSystem->addParticles(newPos, newVel);

		// Read back the data from the particles.
		if ( mParticleSystem->lock() )
		{
			Vector<Point3F> positions = mParticleSystem->readParticles();
			for(int i = 0; i < positions.size(); i++)
				Con::printf("Particle %d (%f, %f, %f)", i, positions[i].x, positions[i].y, positions[i].z);
		}

		// After ~9 seconds start removing the oldest particle every second. 
		//if ( emitterRemoveTick > 300 )
		//	mParticleSystem->removeParticle(0);

		// After ~9 seconds start removing the oldest 3 particles every second.
		if ( emitterRemoveTick > 300 )
			mParticleSystem->removeParticles(0, 3);

		// Apply a force to the particle system.
		mParticleSystem->applyForce(ParticleEmitter::mWindVelocity);
		emitterTick = 0;
	}
}

void Px3ParticleEmitter::interpolateTick( F32 delta )
{
   // Nothing to do for now!
}

bool Px3ParticleEmitter::onNewDataBlock( GameBaseData *dptr, bool reload )
{
   return false;
}