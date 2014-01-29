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
#include "T3D/physics/physx3/Cloth/px3ClothShape.h"
#include "T3D/physics/physx3/px3.h"
#include "T3D/physics/physx3/px3World.h"
#include "T3D/physics/physx3/px3Plugin.h"
#include "T3D/physics/physx3/px3Cast.h"
#include "T3D/physics/physx3/px3Stream.h"

#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/resourceManager.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"

IMPLEMENT_CO_NETOBJECT_V1(Px3ClothShape);

ConsoleDocClass( Px3ClothShape, 
	"@brief A PhysX 3.3 Cloth Shape"
	"@ingroup Physics\n" );

//-----------------------------------------------------------------------------
// Constructor/Destructor
//-----------------------------------------------------------------------------
Px3ClothShape::Px3ClothShape()
{
	mNetFlags.set( Ghostable | ScopeAlways );
	mTypeMask |= StaticObjectType | StaticShapeObjectType;
	mShapeInstance = NULL;
	mWorld = NULL;
	mScene = NULL;

	mDebugTick=0;
}

Px3ClothShape::~Px3ClothShape()
{
}

//-----------------------------------------------------------------------------
// Cloth Management
//-----------------------------------------------------------------------------

void Px3ClothShape::_createFabric(ClothMesh* cMesh)
{
	// Generate list of triangles.
	Vector<U32>* indexData = cMesh->skinMesh->getIndices();
	cMesh->numTriangles = indexData->size();
	for(int a = 0; a < cMesh->numTriangles; a++)
	{
		U32 val = indexData->operator[](a);
		cMesh->triangles.push_back(val);
	}
		
	// Generate list of vertices.
	cMesh->vertexInfo = cMesh->skinMesh->getVertexInfo();
	for(int a = 0; a < cMesh->vertexInfo.size(); a++)
	{
		TSMesh::TSMeshVertexInfo* vert = &cMesh->vertexInfo[a];

		// Weight ( 0 = fixed, 1 = movable )
		// Bone 0 = Cloth Bone
		// Bone 1 = Rigid Bone
		F32 weight = vert->weight;
		if ( vert->bone == 1 ) weight = 0.0f;
		
		// Generate a list of cloth particles.
		Point3F point = vert->point;
		cMesh->vertices.push_back(physx::PxClothParticle(physx::PxVec3(point.x,
			point.y,
			point.z), weight));

		// Correct the Triangles list to remove duplicates
		// vertex references.
		for(int b = 0; b < cMesh->numTriangles; b++)
		{
			for (int c = 0; c < vert->mapsTo.size(); c++)
			{
				if ( vert->mapsTo[c] == cMesh->triangles[b] )
					cMesh->triangles[b] = a;
			}
		}
	}

	// Set the number of verts for this cloth mesh.
	cMesh->numVertices = cMesh->vertices.size();

	// Create description of cloth to send to PhysX.
	physx::PxClothMeshDesc meshDesc;
	meshDesc.points.data = &cMesh->vertices[0];
	meshDesc.points.count = cMesh->numVertices;
	meshDesc.points.stride = sizeof(physx::PxClothParticle);

	meshDesc.invMasses.data = &cMesh->vertices[0].invWeight;
	meshDesc.invMasses.count = cMesh->numVertices;
	meshDesc.invMasses.stride = sizeof(physx::PxClothParticle);

	meshDesc.triangles.data = &cMesh->triangles[0];
	meshDesc.triangles.count = cMesh->numTriangles / 3;
	meshDesc.triangles.stride = sizeof(physx::PxU32) * 3;
	physx::PxClothFabric* clothFabric = physx::PxClothFabricCreate(*gPhysics3SDK, meshDesc, physx::PxVec3(0, 0, 0));

	// Store Fabric.
	if ( clothFabric )
		cMesh->fabric = clothFabric;
}

void Px3ClothShape::_createCloth(ClothMesh* cMesh)
{
    // Make sure we can change the world.
    mWorld->releaseWriteLock();

	// Create fabric if it doesn't exist.
	if ( cMesh->fabric == NULL )
		_createFabric(cMesh);

	// Apply transform
	physx::PxTransform out;
	QuatF q;
	q.set(getTransform());
	out.q = px3Cast<physx::PxQuat>(q);
	out.p = px3Cast<physx::PxVec3>(getTransform().getPosition());

	// Clear old cloth
	if ( cMesh->cloth )
		_releaseCloth(cMesh);

	// Attempt to create 
	physx::PxCloth* cloth = gPhysics3SDK->createCloth(out, *cMesh->fabric, &cMesh->vertices[0], physx::PxClothFlags());
	if ( cloth )
	{
		// Add cloth to the scene.
		mScene->addActor(*cloth);

		// Store cloth information.
		cMesh->cloth = cloth;

		// Duplicate the mesh vertex array for our own personal use.
		TSMesh::TSMeshVertexArray* meshVertexData = cMesh->skinMesh->getVertexData();
		cMesh->meshVertexData = new TSMesh::TSMeshVertexArray();
		cMesh->meshVertexData->set( NULL, 0, 0, false );
		void *aligned_mem = dMalloc_aligned( meshVertexData->vertSize() * meshVertexData->size(), 16 );
		dMemcpy( aligned_mem, meshVertexData->address(), meshVertexData->mem_size() );
		cMesh->meshVertexData->set( aligned_mem, meshVertexData->vertSize(), meshVertexData->size() );
		cMesh->meshVertexData->setReady( true );

		// Override the vertices in the instance with our new vertex array.
		cMesh->meshInstance->setVertexOverride(cMesh->meshVertexData);

		// Setup default cloth properties.
		// This will later be replaced with per-cloth properties from datablock.
		cMesh->cloth->setClothFlag(physx::PxClothFlag::eSCENE_COLLISION, true);
		cMesh->cloth->setSolverFrequency(120.0f);
		cMesh->cloth->setDampingCoefficient(physx::PxVec3(0.0f));
		cMesh->cloth->setStretchConfig(physx::PxClothFabricPhaseType::eVERTICAL, physx::PxClothStretchConfig(1.0f)); //1.0f
		cMesh->cloth->setStretchConfig(physx::PxClothFabricPhaseType::eHORIZONTAL, physx::PxClothStretchConfig(0.9f)); //0.9f
		cMesh->cloth->setStretchConfig(physx::PxClothFabricPhaseType::eSHEARING, physx::PxClothStretchConfig(0.75f)); // 0.75f
		cMesh->cloth->setStretchConfig(physx::PxClothFabricPhaseType::eBENDING, physx::PxClothStretchConfig(0.5f)); // 0.5f
	}
}

void Px3ClothShape::_releaseCloth(ClothMesh* cMesh)
{
	if ( cMesh->cloth )
	{
		mScene->removeActor( *cMesh->cloth );
		cMesh->cloth->release();
		cMesh->cloth = NULL;
	}
}

void Px3ClothShape::_findClothMeshes()
{
	if ( !mShapeInstance )
		return;

	// Clear any data we have.
	_clearClothMeshes();

	// Populate cloth mesh vector with all meshes we find that start with
	// the word cloth. They aren't actually created until after Torque
	// skins them. This is handled in ProcessTick().
	if ( mClothMeshes.size() == 0 )
	{
		//Vector<TSMesh*> data = mShape->findAllMeshes("cloth");
		Vector<TSShapeInstance::MeshObjectInstance*> data = mShapeInstance->findMeshInstances("Cloth");
		
		for (int i = 0; i < data.size(); i++)
		{
			TSSkinMesh* skinMesh = dynamic_cast<TSSkinMesh*>(data[i]->getMesh(0));
			if ( skinMesh )
			{
				ClothMesh cMesh;
				cMesh.cloth = NULL;
				cMesh.fabric = NULL;
				cMesh.numTriangles = 0;
				cMesh.numVertices = 0;
				cMesh.skinMesh = skinMesh;
				cMesh.meshVertexData = NULL;
				cMesh.meshInstance = data[i];
				mClothMeshes.push_back(cMesh);
			}
		}
	}
}

void Px3ClothShape::_clearClothMeshes()
{
	for ( U32 i = 0; i < mClothMeshes.size(); i++ )
	{
		ClothMesh* cMesh = &mClothMeshes[i];

		// Release Cloth.
		_releaseCloth(cMesh);

		// Release Fabric.
		if ( cMesh->fabric != NULL )
		{
			cMesh->fabric->release();
			cMesh->fabric = NULL;
		}
	}
	mClothMeshes.clear();
}

void Px3ClothShape::setTransform( const MatrixF &mat )
{
   Parent::setTransform( mat );
   setMaskBits( TransformMask );
}

void Px3ClothShape::onPhysicsReset( PhysicsResetEvent reset )
{
	// Store the reset transform for later use.
	if ( reset == PhysicsResetEvent_Store )
		mResetXfm = getTransform();

	// Recreate each cloth at the last reset position.
	mWorld->getPhysicsResults();
	Parent::setTransform( mResetXfm );
	for ( int i = 0; i < mClothMeshes.size(); i++ )
		_createCloth(&mClothMeshes[i]);
}

bool Px3ClothShape::onAdd()
{
	if ( !Parent::onAdd() )
		return false;

	// Set up a 1x1x1 bounding box
	mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
				Point3F(  0.5f,  0.5f,  0.5f ) );

	resetWorldBox();
   
	// Add this object to the scene
	addToScene();

	// Setup the shape.
	createShape();

	// Cloth is only created on the client.
	if ( isClientObject() )
	{
		mWorld = dynamic_cast<Px3World*>( PHYSICSMGR->getWorld( "client" ) );
      
		if ( !mWorld || !mWorld->getScene() )
		{
			Con::errorf( "Px3Cloth::onAdd() - PhysXWorld not initialized... cloth disabled!" );
			return true;
		}

		// Find all cloth meshes within the shape.
		_findClothMeshes();

		mResetXfm = getTransform();
		mScene = mWorld->getScene();
		setProcessTick(true);

		PhysicsPlugin::getPhysicsResetSignal().notify( this, &Px3ClothShape::onPhysicsReset, 1053.0f );
	}

	return true;
}

void Px3ClothShape::onRemove()
{
	// Remove this object from the scene
	removeFromScene();

	if ( isClientObject() )
	{
		// Clear any cloth meshes + fabric.
		_clearClothMeshes();

		// Remove physics plugin callback.
		PhysicsPlugin::getPhysicsResetSignal().remove( this, &Px3ClothShape::onPhysicsReset );
	}

	// Remove our TSShapeInstance
	if ( mShapeInstance )
		SAFE_DELETE( mShapeInstance );

	Parent::onRemove();
}

void Px3ClothShape::processTick( const Move *move )
{
	if ( !mShapeInstance )
		return;

    // Update bounds.
    if ( mWorld->isEnabled() )
    {
		// Temporary Wind Vector Hack.
        physx::PxVec3 windVec( 5.0f, 2.0f, 1.0f );
      
		bool displayDebug = false;
		mDebugTick++;
		if ( mDebugTick > 32 )
		{
			displayDebug = true;
			mDebugTick = 0;
		}

		// Loop through each cloth mesh we have listed.
		for ( int i = 0; i < mClothMeshes.size(); i++ )
		{
			// Check if it's actualy physx representation has 
			// been made yet.
			ClothMesh* cMesh = &mClothMeshes[i];
			if ( cMesh->cloth == NULL ) 
			{
				if ( cMesh->skinMesh->hasSkinned() )
					_createCloth(cMesh);
				continue;
			}

			// Read the latest updates from PhysX.
			physx::PxClothParticleData* pData = cMesh->cloth->lockParticleData(physx::PxDataAccessFlag::eREADABLE);
			if ( pData != NULL )
			{
				for(int a = 0; a < cMesh->numVertices; a++)
				{
					// The particle represents a single vertex of the cloth.
					physx::PxClothParticle* particle = &pData->particles[a];

					// Unfortunately there are a number of duplicate vertices
					// and they all have to be updated.
					Vector<U32> vertMap = cMesh->vertexInfo[a].mapsTo;
					for(int b = 0; b < vertMap.size(); b++)
					{
						TSMesh::__TSMeshVertexBase* vert = &(*cMesh->meshVertexData)[vertMap[b]];
						vert->vert(Point3F(particle->pos.x, particle->pos.y, particle->pos.z));
					}
				}
			}
			// Unlock PhysX particle data.
			pData->unlock();

			// Add Wind Acceleration, and updated vertex buffer.
			cMesh->cloth->setExternalAcceleration(windVec);
		}
   }
}

//-----------------------------------------------------------------------------
// Network
//-----------------------------------------------------------------------------
U32 Px3ClothShape::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if ( stream->writeFlag( mask & UpdateMask ) )
   {
      stream->write( mShapeFile );

      // Allow the server object a chance to handle a new shape
      createShape();
   }

   return retMask;
}

void Px3ClothShape::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform( mObjToWorld );
   }

   if ( stream->readFlag() )  // UpdateMask
   {
      stream->read( &mShapeFile );

      if ( isProperlyAdded() )
         createShape();

	  // Attempt to make cloth.
	  _findClothMeshes();
   }
}

//-----------------------------------------------------------------------------
// Static Shape Functions
//-----------------------------------------------------------------------------

void Px3ClothShape::initPersistFields()
{
   addGroup( "Rendering" );
	   addField( "shapeFile",      TypeStringFilename, Offset( mShapeFile, Px3ClothShape ),
		  "The path to the DTS shape file." );
   endGroup( "Rendering" );

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void Px3ClothShape::inspectPostApply()
{
	Parent::inspectPostApply();
	setMaskBits( UpdateMask );
}

void Px3ClothShape::createShape()
{
   if ( mShapeFile.isEmpty() )
      return;

   // If this is the same shape then ncreateShapeo reason to update it
   if ( mShapeInstance && mShapeFile.equal( mShape.getPath().getFullPath(), String::NoCase ) )
      return;

   // Clean up our previous shape
   if ( mShapeInstance )
      SAFE_DELETE( mShapeInstance );
   mShape = NULL;

   // Attempt to get the resource from the ResourceManager
   mShape = ResourceManager::get().load( mShapeFile );

   if ( !mShape )
   {
      Con::errorf( "Px3ClothShape::createShape() - Unable to load shape: %s", mShapeFile.c_str() );
      return;
   }

   // Attempt to preload the Materials for this shape
   if ( isClientObject() && 
        !mShape->preloadMaterialList( mShape.getPath() ) && 
        NetConnection::filesWereDownloaded() )
   {
      mShape = NULL;
      return;
   }

   // Update the bounding box
   mObjBox = mShape->bounds;
   resetWorldBox();
   setRenderTransform(mObjToWorld);

   // Create the TSShapeInstance
   mShapeInstance = new TSShapeInstance( mShape, isClientObject() );
}

void Px3ClothShape::prepRenderImage( SceneRenderState *state )
{
   // Make sure we have a TSShapeInstance
   if ( !mShapeInstance )
      return;

   // Calculate the distance of this object from the camera
   Point3F cameraOffset;
   getRenderTransform().getColumn( 3, &cameraOffset );
   cameraOffset -= state->getDiffuseCameraPosition();
   F32 dist = cameraOffset.len();
   if ( dist < 0.01f )
      dist = 0.01f;

   // Set up the LOD for the shape
   F32 invScale = ( 1.0f / getMax( getMax( mObjScale.x, mObjScale.y ), mObjScale.z ) );

   mShapeInstance->setDetailFromDistance( state, dist * invScale );

   // Make sure we have a valid level of detail
   if ( mShapeInstance->getCurrentDetail() < 0 )
      return;

   // GFXTransformSaver is a handy helper class that restores
   // the current GFX matrices to their original values when
   // it goes out of scope at the end of the function
   GFXTransformSaver saver;

   // Set up our TS render state      
   TSRenderState rdata;
   rdata.setSceneState( state );
   rdata.setFadeOverride( 1.0f );

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   LightQuery query;
   query.init( getWorldSphere() );
   rdata.setLightQuery( &query );

   // Set the world matrix to the objects render transform
   MatrixF mat = getRenderTransform();
   mat.scale( mObjScale );
   GFX->setWorldMatrix( mat );

   // Animate the the shape
   mShapeInstance->animate();

   // Allow the shape to submit the RenderInst(s) for itself
   mShapeInstance->render( rdata );
}