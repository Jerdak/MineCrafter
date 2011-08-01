// Minecrafter.cpp : Defines the entry point for the console application.
//

#include "OcTree.h"
#include "Mesh.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <3DIO/3DIO.h>

using namespace tdio_library;
using namespace std;



///The one and only mesh
Mesh mesh;

//Usage and exit
void uae(){
	printf("Usage: Minecrafter <obj file> <out file>\n");
	exit(1);
}




/** Load mesh file (only supports .obj currently)
*/
void Load(string sFileIn){
	printf("Loading mesh...");
	mesh.object = new Object<TOBJ>();
	if(!Reader::ReadOBJ(sFileIn.c_str(),(*(Object<TOBJ>*)mesh.object))){
		printf("  - Could not load obj file %s\n",sFileIn.c_str());
		exit(1);
	}
	
	mesh.nFace = mesh.object->GetNumFaces();
	mesh.faces = mesh.object->GetFaces();

	mesh.nVtx = mesh.object->GetNumVertices();
	mesh.vtx = mesh.object->GetVerticesAsVectors();


	printf("Done.\n");
}

/** Build mesh metrics for reparameterization
*/
void BuildMetrics(){
	mesh.min = mesh.max = mesh.center = mesh.range = vector3::ZERO;

	printf("Generating metrics...");
	for(int v = 0; v < mesh.nVtx; v++){
		if(v==0){
			mesh.min = mesh.max = mesh.vtx[v];
		} else {
			mesh.min = vector3::Min(mesh.min,mesh.vtx[v]);
			mesh.max = vector3::Max(mesh.max,mesh.vtx[v]);
		}
	}
	mesh.center /= 2.0f;
	printf("Done.\n");

	printf("  - Max: %s\n",mesh.max.toString().c_str());
	printf("  - Min: %s\n",mesh.min.toString().c_str());
	printf("  - Center: %s\n",mesh.center.toString().c_str());

	mesh.range = mesh.max - mesh.min;
	printf("  - Range: %s\n",mesh.range.toString().c_str());
}

/** Reparameterize our mesh
	@notes
		Data are normalized to the largest axis in the mesh which
		preserves relative scale.

		Data are then scaled to the largest allowable height in minecraft.
*/
void Reparameterize(){
	mesh.heightLimit = 128.0;

	if (mesh.range.x > mesh.range.y && mesh.range.x > mesh.range.z)
		mesh.primaryAxis = 0;
	else if(mesh.range.y > mesh.range.z)
		mesh.primaryAxis = 1;
	else
		mesh.primaryAxis = 2;
	
	printf("Reparameterizing vertices...");
	for(int v = 0; v < mesh.nVtx; v++){
		mesh.vtx[v] = ((mesh.vtx[v] - mesh.min)/(mesh.range.cell[mesh.primaryAxis])) * mesh.heightLimit;
	}
	mesh.min = vector3::ZERO;
	mesh.max = ((mesh.max - mesh.min) / mesh.range.cell[mesh.primaryAxis])* mesh.heightLimit;
	mesh.range = mesh.max - mesh.min;
	printf("  - New Max: %s\n",mesh.max.toString().c_str());
	printf("  - New Min: %s\n",mesh.min.toString().c_str());
	printf("  - New Range: %s\n",mesh.range.toString().c_str());
	printf("Done.\n");
}
int main(int argc, char* argv[])
{
	if(argc != 3)uae();
	string sFileIn = argv[1];
	string sFileOut = argv[2];

	Load(sFileIn);
	BuildMetrics();
	Reparameterize();

	{	//Build and clear octree
		OcTree oct(&mesh);
		oct.Build();
		oct.Save(sFileOut);
		oct.Clear();
	}
	//Very special deletion.  When mesh.vtx is created it calls mesh.object.GetVerticesAsVectors(),
	//GetVerticesAsVectors() creates a new vertex array and it dumps its internal pointer leaving it to us to clear the memory.
	if(mesh.vtx){delete [] mesh.vtx; mesh.vtx = NULL;}
	return 0;
}

