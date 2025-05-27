#if !defined(ASSETS_H)

struct vertex 
{
    v3 Pos;
    v2 UV;
};

struct texture
{
    i32 Width;
    i32 Height;
    u32* Texels;
};

struct mesh
{
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
    u32 VertexCount;
    u32 TextureIndex;
};


struct model
{
    u32 NumMeshes;
    mesh* MeshArray;
    u32 NumTextures;
    texture* TextureArray;

    u32 VertexCount;
    vertex* VertexArray;
    u32 IndexCount;
    u32* IndexArray;

};

#define ASSETS_H
#endif