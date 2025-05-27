
char* CombineStrings(const char* StringA, const char* StringB)
{
	char* Result = 0;

	mm LengthA = strlen(StringA);
	mm LengthB = strlen(StringB);
	Result = (char*)malloc(sizeof(char) * (LengthA + LengthB + 1));

	memcpy(Result, StringA, LengthA * sizeof(char));
	memcpy(Result + LengthA, StringB, LengthB * sizeof(char));
	Result[LengthA + LengthB] = 0;

	return Result;
}

/*model AssetLoadModel(char* FolderPath, char* FileName)
{
	model Result = {};

	char* FilePath = CombineStrings(FolderPath, FileName);

	Assimp::Importer Importer;
	const aiScene* Scene = Importer.ReadFile(FilePath, aiProcess_Triangulate | aiProcess_OptimizeMeshes);

	free(FilePath);

	if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
	{
		const char* Error = Importer.GetErrorString();
		InvalidCodePath;
	}

	u32* TextureMappingTable = (u32*)malloc(sizeof(u32) * Scene->mNumMaterials);
	for (u32 MaterialID = 0; MaterialID < Scene->mNumMaterials; ++MaterialID) 
	{
		aiMaterial* CurrMaterial = Scene->mMaterials[MaterialID];

		if (CurrMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) 
		{
			TextureMappingTable[MaterialID] = Result.NumTextures;
			Result.NumTextures += 1;
		}
		else 
		{
			TextureMappingTable[MaterialID] = 0xFFFFFFFF;
		}

	}

	Result.TextureArray = (texture*)malloc(sizeof(texture) * Result.NumTextures);

	u32 CurrTextureID = 0;
	for (u32 MaterialID = 0; MaterialID < Scene->mNumMaterials; ++MaterialID)
	{
		aiMaterial* CurrMaterial = Scene->mMaterials[MaterialID];

		if (CurrMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString TexturePath = {};
			CurrMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &TexturePath);

			texture* CurrTexture = Result.TextureArray + CurrTextureID;

			i32 NumChannels = 0;
			char* FullTexturePath = CombineStrings(FolderPath, TexturePath.C_Str()); 
			u32* UnFlippedTexels = (u32*)stbi_load(FullTexturePath, &CurrTexture->Width, &CurrTexture->Height, &NumChannels, 4);
			free(FullTexturePath);

			CurrTexture->Texels = (u32*)malloc(sizeof(u32) * CurrTexture->Width * CurrTexture->Height);
			for (u32 Y = 0; Y < CurrTexture->Height; ++Y) 
			{
				for (u32 X = 0; X < CurrTexture->Width; ++X)
				{
					CurrTexture->Texels[Y * CurrTexture->Width + X] = UnFlippedTexels[(CurrTexture->Height -1 - Y) * CurrTexture->Width + X];
				}
			}

			CurrTextureID += 1;
		}

	}

	Result.NumMeshes = Scene->mNumMeshes;
	Result.MeshArray = (mesh*)malloc(sizeof(mesh) * Result.NumMeshes);

	for (u32 MeshID = 0; MeshID < Result.NumMeshes; ++MeshID)
	{
		aiMesh* srcMesh = Scene->mMeshes[MeshID];
		mesh* dstMesh = Result.MeshArray + MeshID;

		dstMesh->TextureIndex = TextureMappingTable[srcMesh->mMaterialIndex];

		dstMesh->IndexOffset = Result.IndexCount;
		dstMesh->VertexOffset = Result.VertexCount;

		dstMesh->IndexCount = srcMesh->mNumFaces * 3;
		dstMesh->VertexCount = srcMesh->mNumVertices;

		Result.IndexCount += dstMesh->IndexCount;
		Result.VertexCount += dstMesh->VertexCount;
	}

	Result.VertexArray = (vertex*)malloc(sizeof(vertex) * Result.VertexCount);
	Result.IndexArray = (u32*)malloc(sizeof(u32) * Result.IndexCount);


	f32 MinDistAxisX = FLT_MAX;
	f32 MaxDistAxisX = FLT_MIN;
	f32 MinDistAxisY = FLT_MAX;
	f32 MaxDistAxisY = FLT_MIN;
	f32 MinDistAxisZ = FLT_MAX;
	f32 MaxDistAxisZ = FLT_MIN;

	for (u32 MeshID = 0; MeshID < Result.NumMeshes; ++MeshID)
	{
		aiMesh* srcMesh = Scene->mMeshes[MeshID];
		mesh* dstMesh = Result.MeshArray + MeshID;

		for (u32 VertexID = 0; VertexID < dstMesh->VertexCount; ++VertexID) 
		{
			vertex* CurrVertex = Result.VertexArray + dstMesh->VertexOffset + VertexID;
			CurrVertex->Pos = V3(srcMesh->mVertices[VertexID].x,
								srcMesh->mVertices[VertexID].y,
								srcMesh->mVertices[VertexID].z);

			MinDistAxisX = std::min(CurrVertex->Pos.x, MinDistAxisX);
			MaxDistAxisX = max(CurrVertex->Pos.x, MaxDistAxisX);
			MinDistAxisY = std::min(CurrVertex->Pos.y, MinDistAxisY);
			MaxDistAxisY = max(CurrVertex->Pos.y, MaxDistAxisY);
			MinDistAxisZ = std::min(CurrVertex->Pos.z, MinDistAxisZ);
			MaxDistAxisZ = max(CurrVertex->Pos.z, MaxDistAxisZ);

			if (srcMesh->mTextureCoords[0]) {
				CurrVertex->UV = V2(srcMesh->mTextureCoords[0][VertexID].x,
					srcMesh->mTextureCoords[0][VertexID].y);
			}
			else
			{
				CurrVertex->UV = V2(0, 0);
			}
		}

		for (u32 FaceID = 0; FaceID < srcMesh->mNumFaces; ++FaceID) 
		{
			u32* CurrIndices = Result.IndexArray + dstMesh->IndexOffset + FaceID * 3;
			CurrIndices[0] = srcMesh->mFaces[FaceID].mIndices[0];
			CurrIndices[1] = srcMesh->mFaces[FaceID].mIndices[1];
			CurrIndices[2] = srcMesh->mFaces[FaceID].mIndices[2];
		}
	}

	f32 MiddleCoordX = (MinDistAxisX + MaxDistAxisX) / 2.0f;
	f32 MiddleCoordY = (MinDistAxisY + MaxDistAxisY) / 2.0f;
	f32 MiddleCoordZ = (MinDistAxisZ + MaxDistAxisZ) / 2.0f;

	f32 DistX = abs(MinDistAxisX) + abs(MaxDistAxisX);
	f32 DistY = abs(MinDistAxisY) + abs(MaxDistAxisY);
	f32 DistZ = abs(MinDistAxisZ) + abs(MaxDistAxisZ);

	f32 OneOverMaxDist = 1.0f / max(DistX, max(DistY, DistZ));

	for (u32 VertexID = 0; VertexID < Result.VertexCount; ++VertexID) 
	{
		vertex* CurrVertex = Result.VertexArray + VertexID;
		CurrVertex->Pos = (CurrVertex->Pos - V3(MiddleCoordX, MiddleCoordY, MiddleCoordZ)) * OneOverMaxDist;
	}
	
	return Result;
}*/