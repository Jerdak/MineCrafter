#ifndef __OCTREE_H__
#define __OCTREE_H__

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <3DIO/3DIO.h>
#include "Mesh.h"

/** Octree Node
*/
struct OcNode {
	///Parent (not really used.)
	OcNode* parent;

	///Boundingbox for this particular node
	tdio_library::AABB bb;

	///Child nodes
	std::vector<OcNode*> children;
	
	///Index of mesh faces
	std::vector<int> faces;
};

/** VERY basic Octree
	@descip
		An octree is a simple node structure where each node divides itself in
		to 8 same sized blocks until some metric is met.  In the case of mine craft
		that metric is that the block is 1,1,1 in size.

		Data in Minecraft are limited to a height of 128 units.  Because this limits the height it
		also limits the x/z plane to a max of 128 units as well (in the future we'll allow for
		height independent scaling).

		Each unit block contains a list of faces intersected by that particular block.  In this way we
		can get minecraft block data without requiring a dense point cloid for the mesh.
	@todo
		-  Add a '% enclosed' value that dictates whether a face is in a box based on how much it is present.

*/
class OcTree {
public:
	OcTree(Mesh *inputMesh):mSplitNodeCount(0),mSplitFaceCount(0),mMesh(inputMesh),mRoot(NULL){}
	~OcTree(){}

	/** Build tree
	*/
	void Build(){
		using namespace tdio_library;
		using namespace std;

		printf("Building tree...");
		mRoot = new OcNode[1];
		mRoot->parent = NULL;
		mRoot->bb = AABB(vector3(0,0,0),vector3(128,128,128));
		for(int i = 0; i < mMesh->nFace; i++)mRoot->faces.push_back(i);

		SplitNode(mRoot);
		printf("Complete[%d,%d]\n",mSplitNodeCount,mSplitFaceCount);
	}

	/** Save tree to file (read by worldFromObj.py)
	*/
	void Save(const std::string sFileOut){
		if(!mRoot){
			printf("Saving tree failed, tree not yet built.\n");
			return;
		}
		printf("Saving tree...");
		using namespace tdio_library;
		using namespace std;

		mFileStream = fopen(sFileOut.c_str(),"w");
		fprintf(mFileStream,"--%s---\n",mRoot->bb.GetPos().toString().c_str());
		for(int f = 0; f < mRoot->faces.size(); f++){
			fprintf(mFileStream,"f %d\n",mRoot->faces[f]);
		}
		for(int c = 0; c < mRoot->children.size(); c++){
			DumpNodeToStream(mRoot->children[c]);
		}
		fclose(mFileStream);
		printf("Complete.\n");
	}

	/** Clear tree
	*/
	void Clear(){
		printf("Clearing tree nodes...");
		if(!mRoot)return;

		for(int c = 0; c < mRoot->children.size(); c++){
			OcNode *tmp = mRoot->children[c];
			DeleteNode(tmp);
			if(tmp){delete tmp; tmp = NULL;}
		}
		printf("Complete\n");
	}

protected:
	/** Split nodes equally in to octants
	*/
	void SplitNode(OcNode *node){
		using namespace tdio_library;
		using namespace std;

		//Nodes are split evenly so a single unit length in any direction means we've hit our smallest node.
		if(node->bb.GetSize().x <= 1.0f)return;
		
		//We can also ignore empty bbs
		if(node->faces.size() <= 0)return;

		AABB bb[8];
		vector3 hSize = node->bb.GetHalfSize();

		//Bounding box values are taken as half heights from the parent box.
		bb[0] = AABB(node->bb.GetPos(),hSize);
		bb[1] = AABB(node->bb.GetPos() + vector3(hSize.x,0,0),hSize);
		bb[2] = AABB(node->bb.GetPos() + vector3(0,hSize.y,0),hSize);
		bb[3] = AABB(node->bb.GetPos() + vector3(hSize.x,hSize.y,0),hSize);
		bb[4] = AABB(node->bb.GetPos() + vector3(0,0,hSize.z),hSize);
		bb[5] = AABB(node->bb.GetPos() + vector3(hSize.x,0,hSize.z),hSize);
		bb[6] = AABB(node->bb.GetPos() + vector3(0,hSize.y,hSize.z),hSize);
		bb[7] = AABB(node->bb.GetPos() + vector3(hSize.x,hSize.y,hSize.z),hSize);


		OcNode **child = new OcNode*[8];
		for(int i = 0; i < 8; i++){
			child[i] = new OcNode;
			node->children.push_back(child[i]);

			child[i]->parent = node;
			child[i]->bb = bb[i];

			for(int f = 0; f < node->faces.size(); f++){
				vector3 v0 = mMesh->vtx[mMesh->faces[node->faces[f]].verts[0]];
				vector3 v1 = mMesh->vtx[mMesh->faces[node->faces[f]].verts[1]];
				vector3 v2 = mMesh->vtx[mMesh->faces[node->faces[f]].verts[2]];

				//Test if face crosses or is inside of this child's bounding box.
				if(child[i]->bb.ContainsFace(v0,v1,v2)){
					child[i]->faces.push_back(node->faces[f]);
					mSplitFaceCount++;
				}
			}
			mSplitNodeCount++;
		}
		node->faces.clear();
		for(int i = 0; i < 8; i++){
			SplitNode(child[i]);
		}
	}
	
	/** Dump tree node to file output stream.
	*/
	void DumpNodeToStream(OcNode *node){
		using namespace tdio_library;
		using namespace std;

		float color = 0;
		
		rgb_l *colors = mMesh->object->GetColors();

		fprintf(mFileStream,"g %s\n",node->bb.GetPos().toString().c_str());
		for(int f = 0; f < node->faces.size(); f++){
			fprintf(mFileStream,"f %d\n",node->faces[f]);
			if(colors){
				color += colors[mMesh->faces[node->faces[f]].verts[0]].r * 255;
				color += colors[mMesh->faces[node->faces[f]].verts[1]].r * 255;
				color += colors[mMesh->faces[node->faces[f]].verts[2]].r * 255;
			} 
		}
		if(node->faces.size()!=0){
			color /= node->faces.size() * 3.0f;
			fprintf(mFileStream,"c %f\n",color);
		} else {
			fprintf(mFileStream,"c 0\n");
		}
		for(int c = 0; c < node->children.size(); c++){
			DumpNodeToStream(node->children[c]);
		}
	}

	/** Delete node (recursively deletes all children.
	*/
	void DeleteNode(OcNode *node){
		for(int c = 0; c < node->children.size(); c++){
			OcNode *tmp = node->children[c];
			DeleteNode(tmp);
			
			if(tmp){delete tmp; tmp = NULL;}
		}
	}
private:
	OcNode *mRoot;
	Mesh *mMesh;

	int mSplitNodeCount;
	int mSplitFaceCount;

	FILE *mFileStream;
};

#endif	//__OCTREE_H__