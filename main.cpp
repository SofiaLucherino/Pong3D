#include <math.h>
#include <vector>
#include <windows.h>
#include <mmeapi.h>
#include <thread>

#include "main.h"
#include "graphicsMath.cpp"
#include "clipper.cpp"
#include "assets.cpp"

global global_state GlobalState = {};
local_global v2i ScreenResolution = V2I(1920, 1080);

// How long rectangle will be 1 = square, 12 is standard size of rectangle
global u32 DifficultySize = 12; 

global u32 BallQuality = 100;
global f32 BallDiameter = 0.125f; //0.125;
global f32 BaseSpeed = 3.0f;

v2 NdcToPixels(v2 NdcPos)
{
	v2 Result = {};
	Result = 0.5f * (NdcPos + V2(1.0f));
	Result = Result * V2(GlobalState.FrameBufferWidth - 1, GlobalState.FrameBufferHeight - 1);
	return Result;
}

i64 CrossProduct2d(v2i A, v2i B)
{
	i64 Result = (i64(A.x) * i64(B.y)) - (i64(A.y) * i64(B.x));
	return Result;
}

v3 ColorU32ToRGB(u32 Color) 
{
	v3 Result = {};
	Result.r = (Color >> 16) & 0xFF;
	Result.g = (Color >> 8) & 0xFF;
	Result.b = (Color) & 0xFF;
	Result /= 255.0f;
	return Result;
}

v3_x4 ColorI32X4ToRGB(i32_x4 Color)
{
	v3_x4 Result = {};
	Result.b = F32X4((Color >> 16) & I32X4(0xFF));
	Result.g = F32X4((Color >> 8) & I32X4(0xFF));
	Result.r = F32X4((Color >> 0) & I32X4(0xFF));
	Result /= F32X4(255.0f);
	return Result;
}

u32 RGBToU32(v3 Color) 
{
	u32 Result = {};
	Result = ((0xFF) << 24) | (((u32)(Color.r * 255)) << 16) | (((u32)(Color.g * 255)) << 8) | ((u32)(Color.b * 255));

	return Result;
}

i32_x4 ColorRGBToI32X4(v3_x4 Color)
{
	i32_x4 Result = {};
	v3_x4 ColorTemp = F32X4(255.0f) * Color;
	Result = (I32X4(0xFF) << 24) | ((I32X4(ColorTemp.r)) << 16) | ((I32X4(ColorTemp.g)) << 8) | (I32X4(ColorTemp.b));

	return Result;
}

void DrawTriangle(clip_vertex Vertex0, clip_vertex Vertex1, clip_vertex Vertex2
	, texture Texture, sampler Sampler)
{
	Vertex0.Pos.w = 1.0f / Vertex0.Pos.w;
	Vertex1.Pos.w = 1.0f / Vertex1.Pos.w;
	Vertex2.Pos.w = 1.0f / Vertex2.Pos.w;

	Vertex0.Pos.xyz *= Vertex0.Pos.w;
	Vertex1.Pos.xyz *= Vertex1.Pos.w;
	Vertex2.Pos.xyz *= Vertex2.Pos.w;

	Vertex0.UV *= Vertex0.Pos.w;
	Vertex1.UV *= Vertex1.Pos.w;
	Vertex2.UV *= Vertex2.Pos.w;

	v2 PointAF = NdcToPixels(Vertex0.Pos.xy);
	v2 PointBF = NdcToPixels(Vertex1.Pos.xy);
	v2 PointCF = NdcToPixels(Vertex2.Pos.xy);

	i32 MinX = min(min((i32)PointAF.x, (i32)PointBF.x), (i32)PointCF.x);
	i32 MinY = min(min((i32)PointAF.y, (i32)PointBF.y), (i32)PointCF.y);
	
	i32 MaxX = max(max((i32)round(PointAF.x), (i32)round(PointBF.x)), (i32)round(PointCF.x));
	i32 MaxY = max(max((i32)round(PointAF.y), (i32)round(PointBF.y)), (i32)round(PointCF.y));

	#if 0
	MinX = max(0, MinX);
	MinX = min(MinX, GlobalState.FrameBufferWidth - 1);

	MinY = max(0, MinY);
	MinY = min(MinY, GlobalState.FrameBufferHeight - 1);

	MaxX = max(0, MaxX);
	MaxX = min(MaxX, GlobalState.FrameBufferWidth - 1);

	MaxY = max(0, MaxY);
	MaxY = min(MaxY, GlobalState.FrameBufferHeight - 1);
	#endif

	v2i PointA = V2I_F24_8(PointAF);
	v2i PointB = V2I_F24_8(PointBF);
	v2i PointC = V2I_F24_8(PointCF);


	v2i Edge0 = PointB - PointA;
	v2i Edge1 = PointC - PointB;
	v2i Edge2 = PointA - PointC;

	b32 isTopLeft0 = (Edge0.y > 0 || Edge0.x > 0 && Edge0.y == 0);
	b32 isTopLeft1 = (Edge1.y > 0 || Edge1.x > 0 && Edge1.y == 0);
	b32 isTopLeft2 = (Edge2.y > 0 || Edge2.x > 0 && Edge2.y == 0);

	f32_x4 BaryCentricDiv = F32X4(256.0f / f32(CrossProduct2d(PointB - PointA, PointC - PointA)));

	i32_x4 Edge0DiffX = I32X4(Edge0.y);
	i32_x4 Edge0DiffY = I32X4(-Edge0.x);
	i32_x4 Edge1DiffX = I32X4(Edge1.y);
	i32_x4 Edge1DiffY = I32X4(-Edge1.x);
	i32_x4 Edge2DiffX = I32X4(Edge2.y);
	i32_x4 Edge2DiffY = I32X4(-Edge2.x);

	i32_x4 Edge0RowY = {};
	i32_x4 Edge1RowY = {};
	i32_x4 Edge2RowY = {};

	{
		v2i StartPos = V2I_F24_8(V2(MinX, MinY) + V2(0.5f, 0.5f));

		i64 Edge0RowY64 = CrossProduct2d(StartPos - PointA, Edge0);
		i64 Edge1RowY64 = CrossProduct2d(StartPos - PointB, Edge1);
		i64 Edge2RowY64 = CrossProduct2d(StartPos - PointC, Edge2);

		i32 Edge0RowY32 = i32((Edge0RowY64 + Sign(Edge0RowY64) * 128) / 256) - (isTopLeft0 ? 0 : -1);
		i32 Edge1RowY32 = i32((Edge1RowY64 + Sign(Edge1RowY64) * 128) / 256) - (isTopLeft1 ? 0 : -1);
		i32 Edge2RowY32 = i32((Edge2RowY64 + Sign(Edge2RowY64) * 128) / 256) - (isTopLeft2 ? 0 : -1);

		Edge0RowY = I32X4(Edge0RowY32) + (I32X4(0, 1, 2, 3) * Edge0DiffX);
		Edge1RowY = I32X4(Edge1RowY32) + (I32X4(0, 1, 2, 3) * Edge1DiffX);
		Edge2RowY = I32X4(Edge2RowY32) + (I32X4(0, 1, 2, 3) * Edge2DiffX);
	}
	Edge0DiffX = Edge0DiffX * I32X4(4);
	Edge1DiffX = Edge1DiffX * I32X4(4);
	Edge2DiffX = Edge2DiffX * I32X4(4);


	for (int Y = { MinY }; Y <= MaxY; ++Y)
	{
		i32_x4 Edge0RowX = Edge0RowY;
		i32_x4 Edge1RowX = Edge1RowY;
		i32_x4 Edge2RowX = Edge2RowY;

		for (int X = { MinX }; X <= MaxX; X += 4)
		{
			u32 PixelID = (Y * GlobalState.FrameBufferStride) + X;

			f32* DepthPtr = GlobalState.DepthBuffer + PixelID;
			f32_x4 PixelDepths = F32X4Load(DepthPtr);

			i32* ColorPtr = (i32*)GlobalState.FrameBufferPixels + PixelID;
			i32_x4 PixelColors = I32X4Load(ColorPtr);

			i32_x4 EdgeMask = (Edge0RowX | Edge1RowX | Edge2RowX) >= 0;
			if (_mm_movemask_epi8(EdgeMask.Vals)) {

				f32_x4 T0 = -F32X4(Edge1RowX) * BaryCentricDiv;
				f32_x4 T1 = -F32X4(Edge2RowX) * BaryCentricDiv;
				f32_x4 T2 = -F32X4(Edge0RowX) * BaryCentricDiv;

				f32_x4 Depth = Vertex0.Pos.z + T1 * (Vertex1.Pos.z - Vertex0.Pos.z) + T2 * (Vertex2.Pos.z - Vertex0.Pos.z);
				i32_x4 DepthMask = I32X4ReInterpret(Depth < PixelDepths);


				f32_x4 OneOverW = T0 * Vertex0.Pos.w + T1 * Vertex1.Pos.w + T2 * Vertex2.Pos.w;

				v2_x4 UV = T0 * V2X4(Vertex0.UV) + T1 * V2X4(Vertex1.UV) + T2 * V2X4(Vertex2.UV);
				UV /= OneOverW;

				i32_x4 TextureColor = I32X4(0);

				switch (Sampler.Type)
				{
					case(SamplerType_Nearest):
					{
						i32_x4 TexelX = I32X4(Floor(UV.x * (Texture.Width - 1)));
						i32_x4 TexelY = I32X4(Floor(UV.y * (Texture.Height - 1)));

						TexelX = Max(Min(TexelX, I32X4(Texture.Width - 1)), I32X4(0));
						TexelY = Max(Min(TexelY, I32X4(Texture.Height - 1)), I32X4(0));

						i32_x4 TexelMask = (TexelX >= 0 & TexelX < Texture.Width
							& TexelY >= 0 & TexelY < Texture.Height);

						i32_x4 TexelOffsets = TexelY * I32X4(Texture.Width) + TexelX;

						i32_x4 TrueCase = I32X4Gather((i32*)Texture.Texels, TexelOffsets);
						i32_x4 FalseCase = I32X4((i32)0xffff00ff);

						TextureColor = (TexelMask & TrueCase) + AndNot(TexelMask, FalseCase);
					} break;
					case(SamplerType_Bilinear):
					{
#if 0
						v2_x4 TexelV2 = UV * V2X4(Texture.Width, Texture.Height) - V2X4(0.5f, 0.5f);
						v2i_x4 TexelPos[4] = {};
						TexelPos[0] = V2IX4(Floor(TexelV2.x), Floor(TexelV2.y));
						TexelPos[1] = TexelPos[0] + V2IX4(1, 0);
						TexelPos[2] = TexelPos[0] + V2IX4(0, 1);
						TexelPos[3] = TexelPos[0] + V2IX4(1, 1);

						v3_x4 TexelColors[4] = {};

						for (u32 TexelID = { 0 }; TexelID < ArrayCount(TexelPos); TexelID++)
						{
							v2i_x4 CurrTexelPos = TexelPos[TexelID];
							i32_x4 TexelMask = (CurrTexelPos.x >= 0 & CurrTexelPos.x < Texture.Width
								& CurrTexelPos.y >= 0 & CurrTexelPos.y < Texture.Height);

							CurrTexelPos.x = Max(Min(CurrTexelPos.x, I32X4(Texture.Width - 1)), I32X4(0));
							CurrTexelPos.y = Max(Min(CurrTexelPos.y, I32X4(Texture.Height - 1)), I32X4(0));

							i32_x4 TexelOffsets = CurrTexelPos.y * I32X4(Texture.Width) + CurrTexelPos.x;
							i32_x4 TrueCase = I32X4Gather((i32*)Texture.Texels, TexelOffsets);
							i32_x4 FalseCase = I32X4((i32)Sampler.BoarderColor);
							i32_x4 TexelColorsI32 = (TexelMask & TrueCase) + AndNot(TexelMask, FalseCase);

							TexelColors[TexelID] = ColorI32X4ToRGB(TexelColorsI32);
						}
						f32_x4 S = TexelV2.x - Floor(TexelV2.x);
						f32_x4 K = TexelV2.y - Floor(TexelV2.y);

						v3_x4 Interpolated0 = Lerp(TexelColors[0], TexelColors[1], S);
						v3_x4 Interpolated1 = Lerp(TexelColors[2], TexelColors[3], S);
						v3_x4 FinalColor = Lerp(Interpolated0, Interpolated1, K);

						TextureColor = ColorRGBToI32X4(FinalColor);
#else
						v2_x4 TexelV2 = UV * V2X4(Texture.Width, Texture.Height) - V2X4(0.5f, 0.5f);
						v2i_x4 TexelPos[4] = {};
						TexelPos[0] = V2IX4(Floor(TexelV2.x), Floor(TexelV2.y));
						TexelPos[1] = TexelPos[0] + V2IX4(1, 0);
						TexelPos[2] = TexelPos[0] + V2IX4(0, 1);
						TexelPos[3] = TexelPos[0] + V2IX4(1, 1);

						v3_x4 TexelColors[4] = {};

						for (u32 TexelID = { 0 }; TexelID < ArrayCount(TexelPos); TexelID++)
						{
							v2i_x4 CurrTexelPos = TexelPos[TexelID];
							{
								v2_x4 CurrTexelPosF = V2X4(CurrTexelPos);
								v2_x4 Factor = {};
									Factor.x = Floor(CurrTexelPosF.x / F32X4(Texture.Width));
									Factor.y = Floor(CurrTexelPosF.y / F32X4(Texture.Height));
								CurrTexelPosF.x = CurrTexelPosF.x - (F32X4(Texture.Width) * Factor.x);
								CurrTexelPosF.y = CurrTexelPosF.y - (F32X4(Texture.Height) * Factor.y);
								CurrTexelPos = V2IX4(CurrTexelPosF);
							}

							i32_x4 TexelOffsets = CurrTexelPos.y * I32X4(Texture.Width) + CurrTexelPos.x;
							i32_x4 LoadMask = EdgeMask & DepthMask;

							TexelOffsets = (TexelOffsets & LoadMask) + AndNot(LoadMask, I32X4(0));

							i32_x4 TexelColorsI32 = I32X4Gather((i32*)Texture.Texels, TexelOffsets);
							
							TexelColors[TexelID] = ColorI32X4ToRGB(TexelColorsI32);
						}
						f32_x4 S = TexelV2.x - Floor(TexelV2.x);
						f32_x4 K = TexelV2.y - Floor(TexelV2.y);

						v3_x4 Interpolated0 = Lerp(TexelColors[0], TexelColors[1], S);
						v3_x4 Interpolated1 = Lerp(TexelColors[2], TexelColors[3], S);
						v3_x4 FinalColor = Lerp(Interpolated0, Interpolated1, K);

						TextureColor = ColorRGBToI32X4(FinalColor);
#endif 
					} break;
					default:
					{
						InvalidCodePath;
					} break;
				}

				i32_x4 FinalMaskI32 = EdgeMask & DepthMask;
				f32_x4 FinalMaskF32 = F32X4ReInterpret(FinalMaskI32);

				i32_x4 OutputColors = (TextureColor & FinalMaskI32) + AndNot(FinalMaskI32, PixelColors);
				f32_x4 OutputDepth = (Depth & FinalMaskF32) + AndNot(FinalMaskF32, PixelDepths);

				I32X4Store(ColorPtr, OutputColors);
				F32X4Store(DepthPtr, OutputDepth);
			}
			Edge0RowX += Edge0DiffX;
			Edge1RowX += Edge1DiffX;
			Edge2RowX += Edge2DiffX;
		}
		Edge0RowY += Edge0DiffY;
		Edge1RowY += Edge1DiffY;
		Edge2RowY += Edge2DiffY;
	}
}

void DrawTriangle(v4 ModelVertex0, v4 ModelVertex1, v4 ModelVertex2
	, v2 ModelUV0, v2 ModelUV1, v2 ModelUV2
	, texture Texture, sampler Sampler)
{

	clip_result Ping;
	Ping.NumTriangles = 1;
	Ping.Vertices[0] = { ModelVertex0, ModelUV0 };
	Ping.Vertices[1] = { ModelVertex1, ModelUV1 };
	Ping.Vertices[2] = { ModelVertex2, ModelUV2 };

	clip_result Pong = {};

	ClipPolygonToAxis(&Ping, &Pong, ClipAxis_Left);
	ClipPolygonToAxis(&Pong, &Ping, ClipAxis_Right);
	ClipPolygonToAxis(&Ping, &Pong, ClipAxis_Top);
	ClipPolygonToAxis(&Pong, &Ping, ClipAxis_Bottom);
	ClipPolygonToAxis(&Ping, &Pong, ClipAxis_Near);
	ClipPolygonToAxis(&Pong, &Ping, ClipAxis_Far);
	ClipPolygonToAxis(&Ping, &Pong, ClipAxis_W);

	for (int TriangleId = 0; TriangleId < Pong.NumTriangles; ++TriangleId) 
	{
		DrawTriangle(Pong.Vertices[3 * TriangleId + 0], Pong.Vertices[3 * TriangleId + 1],
			Pong.Vertices[3 * TriangleId + 2], Texture, Sampler);
	}
}

LRESULT Win32WindowCallBack(
	HWND WindowHandle,
	UINT Massage,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = {};

	switch (Massage)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	{ GlobalState.IsRunning = false; } break;

	default:
	{ Result = DefWindowProcA(WindowHandle, Massage, WParam, LParam); } break;

	}
	return Result;
};

void DrawModel(model* Model, m4 Transform, sampler Sampler)
{
	v4* TransformedVertices = (v4*)malloc(sizeof(v4) * Model->VertexCount);
	for (u32 VertexId = { 0 }; VertexId < Model->VertexCount; ++VertexId)
	{
		TransformedVertices[VertexId] = (Transform * V4(Model->VertexArray[VertexId].Pos, 1.0f));
	}
	
	for (u32 MeshID = 0; MeshID < Model->NumMeshes; ++MeshID) 
	{
		mesh* CurrentMesh = Model->MeshArray + MeshID;
		for (u32 IndexId = 0; IndexId < CurrentMesh->IndexCount; IndexId += 3)
		{	
			u32 Index0 = Model->IndexArray[CurrentMesh->IndexOffset + IndexId + 0];
			u32 Index1 = Model->IndexArray[CurrentMesh->IndexOffset + IndexId + 1];
			u32 Index2 = Model->IndexArray[CurrentMesh->IndexOffset + IndexId + 2];

			v4 Pos0 = TransformedVertices[CurrentMesh->VertexOffset + Index0];
			v4 Pos1 = TransformedVertices[CurrentMesh->VertexOffset + Index1];
			v4 Pos2 = TransformedVertices[CurrentMesh->VertexOffset + Index2];

			v2 UV0 = Model->VertexArray[CurrentMesh->VertexOffset + Index0].UV;
			v2 UV1 = Model->VertexArray[CurrentMesh->VertexOffset + Index1].UV;
			v2 UV2 = Model->VertexArray[CurrentMesh->VertexOffset + Index2].UV;

			DrawTriangle(Pos0, Pos1, Pos2, UV0, UV1, UV2, Model->TextureArray[CurrentMesh->TextureIndex], Sampler);
		}
	}

	free(TransformedVertices);
}

void DrawObject(modelInWorld Object, m4 Transform, sampler Sampler)
{
	DrawModel(&Object.model, Transform, Sampler);
}

int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	GlobalState.IsRunning = true;

	LARGE_INTEGER timerFrequency = {};
	Assert(QueryPerformanceFrequency(&timerFrequency));

	{

		WNDCLASSA windowClass = {};
		windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = Win32WindowCallBack;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
		windowClass.lpszClassName = "Game";

		if (!RegisterClassA(&windowClass)) { InvalidCodePath; }

		GlobalState.WindowHandle = CreateWindowExA(
			0,
			windowClass.lpszClassName,
			"Pong 3D",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			ScreenResolution.x, ScreenResolution.y,
			NULL,
			NULL,
			hInstance,
			NULL);

		if (!GlobalState.WindowHandle) { InvalidCodePath; }

		GlobalState.DeviceContext = GetDC(GlobalState.WindowHandle);
		if (!GlobalState.DeviceContext) { InvalidCodePath; }
	}

	{
		RECT clientRect = {};
		Assert(GetClientRect(GlobalState.WindowHandle, &clientRect));

		GlobalState.FrameBufferWidth = clientRect.right - clientRect.left;
		GlobalState.FrameBufferHeight = clientRect.bottom - clientRect.top;
		GlobalState.FrameBufferStride = GlobalState.FrameBufferWidth + 3;

		GlobalState.FrameBufferPixels = (u32*)malloc(sizeof(u32) * GlobalState.FrameBufferStride * GlobalState.FrameBufferHeight);
		GlobalState.DepthBuffer = (f32*)malloc(sizeof(f32) * GlobalState.FrameBufferStride * GlobalState.FrameBufferHeight);
	}

	texture Gradient = {};

	texture LogoText = {};
	texture StartText = {};
	texture OptionsText = {};
	texture ExitText = {};

	texture* NumbersTextures[10];

	texture NumZero = {};
	NumbersTextures[0] = &NumZero;
	texture NumOne = {};
	NumbersTextures[1] = &NumOne;
	texture NumTwo = {};
	NumbersTextures[2] = &NumTwo;
	texture NumThree = {};
	NumbersTextures[3] = &NumThree;
	texture NumFour = {};
	NumbersTextures[4] = &NumFour;
	texture NumFive = {};
	NumbersTextures[5] = &NumFive;
	texture NumSix = {};
	NumbersTextures[6] = &NumSix;
	texture NumSeven = {};
	NumbersTextures[7] = &NumSeven;
	texture NumEight = {};
	NumbersTextures[8] = &NumEight;
	texture NumNine = {};
	NumbersTextures[9] = &NumNine;

	u32 GradientBrightColor = 0xFFFFFFFF;
	u32 GradientDarkColor = 0xFF5F5F5F5F;
	sampler Sampler = {};

	//Creating Textures
	{
		Sampler.Type = SamplerType_Nearest;
		Sampler.BoarderColor = GradientBrightColor;

		u32 Blocksize = 4;
		u32 numBlocks = 32;

		Gradient.Width = 256;
		Gradient.Height = 1;
		Gradient.Texels = (u32*)malloc(sizeof(u32*) * Gradient.Height * Gradient.Width);

		for (u32 i = 0; i < Gradient.Width; i++)
		{
			u32 ColorStart = GradientDarkColor;
			u32 ColorAdd = GradientBrightColor - GradientDarkColor;

			float K = i / 255.0f;
			u32 ColorChannelB = ColorStart % 256 + (u32)(K * (ColorAdd % 256));

			GradientDarkColor >> 8;
			u32 ColorChannelG = ColorStart % 256 + (u32)(K * (ColorAdd % 256));

			GradientDarkColor >> 8;
			u32 ColorChannelR = ColorStart % 256 + (u32)(K * (ColorAdd % 256));

			Gradient.Texels[i] = (((u32)0xFF) << 24) + (ColorChannelR << 16) + (ColorChannelG << 8) + ColorChannelB;
		}

		{
			i32 W = 1024;
			i32 H = 256;

			LogoText.Width = W;
			LogoText.Height = H;
			LogoText.Texels = (u32*)malloc(sizeof(u32*) * LogoText.Height * LogoText.Width);

			StartText.Width = W;
			StartText.Height = H;
			StartText.Texels = (u32*)malloc(sizeof(u32*) * StartText.Height * StartText.Width);


			OptionsText.Width = W;
			OptionsText.Height = H;
			OptionsText.Texels = (u32*)malloc(sizeof(u32*) * OptionsText.Height * OptionsText.Width);


			ExitText.Width = W;
			ExitText.Height = H;
			ExitText.Texels = (u32*)malloc(sizeof(u32*) * ExitText.Height * ExitText.Width);

			HDC vhdc = CreateCompatibleDC(GlobalState.DeviceContext);
			if (!vhdc)
			{
				InvalidCodePath;
			}

			HBITMAP LogoBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP StartBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP OptionsBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP ExitBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);

			BITMAPINFO ButtonTextureBitmapInfo = { {sizeof(tagBITMAPINFOHEADER), W, -H, 1, 32, BI_RGB, 0, 0, 0, 0, 0} , {0, 0, 0, 0} };
			HFONT bigFont = CreateFontA(128, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_DONTCARE, "Comic Sans MS");
			SelectObject(vhdc, bigFont);

			SetTextAlign(vhdc, TA_CENTER);
			SetTextColor(vhdc, RGB(255, 0, 0));
			SetBkColor(vhdc, RGB(0, 0, 0));



			texture currTexture = {};
			currTexture.Height = W;
			currTexture.Width = H;
			currTexture.Texels = (u32*)malloc(sizeof(u32*) * currTexture.Height * currTexture.Width);


			SelectObject(vhdc, LogoBitmap);
			TextOut(vhdc, W / 2, H / 4, L"Pong in 3D", 10);
			if (!GetDIBits(vhdc, LogoBitmap, 0, H, currTexture.Texels, &ButtonTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currTexture.Height; i++)
			{
				for (u32 j = 0; j < currTexture.Width; j++)
				{
					u32 currID = i * currTexture.Width + j;
					u32 currColor = currTexture.Texels[currID];

					LogoText.Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					LogoText.Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					LogoText.Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					LogoText.Texels[currID] += 0xFF000000;

					b32 equalBlack = LogoText.Texels[currID] == 0xFF000000;
					b32 equalWhite = LogoText.Texels[currID] == 0xFFFFFFFF;

					LogoText.Texels[currID] = LogoText.Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}


			SetBkColor(vhdc, RGB(255, 255, 255));
			SelectObject(vhdc, StartBitmap);
			TextOut(vhdc, W / 2, H / 4, L"Start", 5);
			if (!GetDIBits(vhdc, StartBitmap, 0, H, currTexture.Texels, &ButtonTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currTexture.Height; i++)
			{
				for (u32 j = 0; j < currTexture.Width; j++)
				{
					u32 currID = i * currTexture.Width + j;
					u32 currColor = currTexture.Texels[currID];

					StartText.Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					StartText.Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					StartText.Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					StartText.Texels[currID] += 0xFF000000;


					b32 equalBlack = StartText.Texels[currID] == 0xFF000000;
					b32 equalWhite = StartText.Texels[currID] == 0xFFFFFFFF;

					StartText.Texels[currID] = StartText.Texels[currID] * !equalBlack + GradientBrightColor * equalBlack;
				}
			}

			SelectObject(vhdc, OptionsBitmap);
			TextOut(vhdc, W / 2, H / 4, L"Options", 7);
			if (!GetDIBits(vhdc, OptionsBitmap, 0, H, currTexture.Texels, &ButtonTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}
			for (u32 i = 0; i < currTexture.Height; i++)
			{
				for (u32 j = 0; j < currTexture.Width; j++)
				{
					u32 currID = i * currTexture.Width + j;
					u32 currColor = currTexture.Texels[currID];

					OptionsText.Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					OptionsText.Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					OptionsText.Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					OptionsText.Texels[currID] += 0xFF000000;


					b32 equalBlack = OptionsText.Texels[currID] == 0xFF000000;
					b32 equalWhite = OptionsText.Texels[currID] == 0xFFFFFFFF;

					OptionsText.Texels[currID] = OptionsText.Texels[currID] * !equalBlack + GradientBrightColor * equalBlack;
				}
			}

			SelectObject(vhdc, ExitBitmap);
			TextOut(vhdc, W / 2, H / 4, L"Exit", 4);
			if (!GetDIBits(vhdc, ExitBitmap, 0, H, currTexture.Texels, &ButtonTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}
			for (u32 i = 0; i < currTexture.Height; i++)
			{
				for (u32 j = 0; j < currTexture.Width; j++)
				{
					u32 currID = i * currTexture.Width + j;
					u32 currColor = currTexture.Texels[currID];

					ExitText.Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					ExitText.Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					ExitText.Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					ExitText.Texels[currID] += 0xFF000000;

					b32 equalBlack = ExitText.Texels[currID] == 0xFF000000;
					b32 equalWhite = ExitText.Texels[currID] == 0xFFFFFFFF;

					ExitText.Texels[currID] = ExitText.Texels[currID] * !equalBlack + GradientBrightColor * equalBlack;
				}
			}


			DeleteObject(LogoBitmap);
			DeleteObject(StartBitmap);
			DeleteObject(OptionsBitmap);
			DeleteObject(ExitBitmap);
			DeleteObject(bigFont);
			DeleteDC(vhdc);
			free(currTexture.Texels);
		}

		{
			i32 W = 256 * 2;
			i32 H = 256 * 3;

			NumZero.Width = W;
			NumZero.Height = H;
			NumZero.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumOne.Width = W;
			NumOne.Height = H;
			NumOne.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumTwo.Width = W;
			NumTwo.Height = H;
			NumTwo.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumThree.Width = W;
			NumThree.Height = H;
			NumThree.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumFour.Width = W;
			NumFour.Height = H;
			NumFour.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumFive.Width = W;
			NumFive.Height = H;
			NumFive.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumSix.Width = W;
			NumSix.Height = H;
			NumSix.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumSeven.Width = W;
			NumSeven.Height = H;
			NumSeven.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumEight.Width = W;
			NumEight.Height = H;
			NumEight.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			NumNine.Width = W;
			NumNine.Height = H;
			NumNine.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			HDC vhdc = CreateCompatibleDC(GlobalState.DeviceContext);
			if (!vhdc)
			{
				InvalidCodePath;
			}

			HBITMAP ZeroBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP OneBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP TwoBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP ThreeBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP FourBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);

			HBITMAP FiveBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP SixBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP SevenBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP EightBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);
			HBITMAP NineBitmap = CreateCompatibleBitmap(GlobalState.DeviceContext, W, H);

			BITMAPINFO ScoreTextureBitmapInfo = { {sizeof(tagBITMAPINFOHEADER), W, -H, 1, 32, BI_RGB, 0, 0, 0, 0, 0} , {0, 0, 0, 0} };
			HFONT bigFont = CreateFontA(H, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_DONTCARE, "Comic Sans MS");
			SelectObject(vhdc, bigFont);

			SetTextAlign(vhdc, TA_CENTER);
			SetTextColor(vhdc, RGB(255, 0, 0));
			SetBkColor(vhdc, RGB(0, 0, 0));



			texture currBitmapTexture = {};
			currBitmapTexture.Height = W;
			currBitmapTexture.Width = H;
			currBitmapTexture.Texels = (u32*)malloc(sizeof(u32*) * H * W);

			texture* WriteTexture = &NumZero;

			SelectObject(vhdc, ZeroBitmap);
			TextOut(vhdc, W / 2, 0, L"0", 1);
			if (!GetDIBits(vhdc, ZeroBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}
			WriteTexture = &NumOne;

			SelectObject(vhdc, OneBitmap);
			TextOut(vhdc, W / 2, 0, L"1", 1);
			if (!GetDIBits(vhdc, OneBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumTwo;

			SelectObject(vhdc, TwoBitmap);
			TextOut(vhdc, W / 2, 0, L"2", 1);
			if (!GetDIBits(vhdc, TwoBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumThree;

			SelectObject(vhdc, ThreeBitmap);
			TextOut(vhdc, W / 2, 0, L"3", 1);
			if (!GetDIBits(vhdc, ThreeBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumFour;

			SelectObject(vhdc, FourBitmap);
			TextOut(vhdc, W / 2, 0, L"4", 1);
			if (!GetDIBits(vhdc, FourBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumFive;

			SelectObject(vhdc, FiveBitmap);
			TextOut(vhdc, W / 2, 0, L"5", 1);
			if (!GetDIBits(vhdc, FiveBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumSix;

			SelectObject(vhdc, SixBitmap);
			TextOut(vhdc, W / 2, 0, L"6", 1);
			if (!GetDIBits(vhdc, SixBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumSeven;

			SelectObject(vhdc, SevenBitmap);
			TextOut(vhdc, W / 2, 0, L"7", 1);
			if (!GetDIBits(vhdc, SevenBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumEight;

			SelectObject(vhdc, EightBitmap);
			TextOut(vhdc, W / 2, 0, L"8", 1);
			if (!GetDIBits(vhdc, EightBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}

			WriteTexture = &NumNine;

			SelectObject(vhdc, NineBitmap);
			TextOut(vhdc, W / 2, 0, L"9", 1);
			if (!GetDIBits(vhdc, NineBitmap, 0, H, currBitmapTexture.Texels, &ScoreTextureBitmapInfo, BI_RGB))
			{
				InvalidCodePath;
			}

			for (u32 i = 0; i < currBitmapTexture.Height; i++)
			{
				for (u32 j = 0; j < currBitmapTexture.Width; j++)
				{
					u32 currID = i * currBitmapTexture.Width + j;
					u32 currColor = currBitmapTexture.Texels[currID];

					WriteTexture->Texels[currID] = 0x00010000 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000100 * (u8)(currColor % 256);
					currColor = currColor >> 8;
					WriteTexture->Texels[currID] += 0x00000001 * (u8)(currColor % 256);

					WriteTexture->Texels[currID] += 0xFF000000;

					b32 equalBlack = WriteTexture->Texels[currID] == 0xFF000000;
					b32 equalWhite = WriteTexture->Texels[currID] == 0xFFFFFFFF;

					WriteTexture->Texels[currID] = WriteTexture->Texels[currID] * !equalBlack + 0xFF000000 * equalBlack;
				}
			}



			DeleteObject(ZeroBitmap);
			DeleteObject(OneBitmap);
			DeleteObject(TwoBitmap);
			DeleteObject(ThreeBitmap);
			DeleteObject(FourBitmap);
			DeleteObject(FiveBitmap);
			DeleteObject(SixBitmap);
			DeleteObject(SevenBitmap);
			DeleteObject(EightBitmap);
			DeleteObject(NineBitmap);
			DeleteObject(bigFont);
			DeleteDC(vhdc);
			free(currBitmapTexture.Texels);
		}
	}

	//CreatingModels
	// RectangleModel
	{
		local_global vertex ModelVertices[] = {
			//FrontFace
			{ V3(-0.03125f, -0.03125f * DifficultySize, -0.5f), V2(0, 0) },
			{ V3(-0.03125f, 0.03125f * DifficultySize, -0.5f), V2(0, 0) },
			{ V3(0.03125f, 0.03125f * DifficultySize, -0.5f), V2(1, 0) },
			{ V3(0.03125f, -0.03125f * DifficultySize, -0.5f), V2(1, 0) },

			//BackFace
			{ V3(-0.03125f, -0.03125f * DifficultySize, 0.5f), V2(0, 0) },
			{ V3(-0.03125f, 0.03125f * DifficultySize, 0.5f), V2(0, 0) },
			{ V3(0.03125f, 0.03125f * DifficultySize, 0.5f), V2(1, 0) },
			{ V3(0.03125f, -0.03125f * DifficultySize, 0.5f), V2(1, 0) }
		};

		local_global u32 ModelIndices[] = {
			//FrontFace
			0, 1, 2,
			2, 3, 0,
			//Backface
			6, 5, 4,
			4, 7, 6,
			//LeftFace
			4, 5, 1,
			1, 0, 4,
			//RightFace
			3, 2, 6,
			6 ,7 ,3,
			//TopFace
			1, 5, 6,
			6, 2, 1,
			//BottomFace
			4, 0, 3,
			3, 7, 4
		};

		local_global mesh Mesh = {};
		Mesh.TextureIndex = 0;

		Mesh.IndexOffset = 0;
		Mesh.IndexCount = ArrayCount(ModelIndices);

		Mesh.VertexOffset = 0;
		Mesh.VertexCount = ArrayCount(ModelVertices);

		GlobalState.rectangle = {};

		GlobalState.rectangle.NumMeshes = 1;
		GlobalState.rectangle.MeshArray = &Mesh;

		GlobalState.rectangle.VertexCount = ArrayCount(ModelVertices);
		GlobalState.rectangle.VertexArray = ModelVertices;

		GlobalState.rectangle.IndexCount = ArrayCount(ModelIndices);
		GlobalState.rectangle.IndexArray = ModelIndices;

		GlobalState.rectangle.NumTextures = 1;
		GlobalState.rectangle.TextureArray = &Gradient;
	}
	//BorderModel
	{
		local_global vertex ModelVertices[] = {
			//FrontFace
			{ V3(-6, -0.03125f, -0.5f), V2(0, 0) },
			{ V3(-6, 0.03125f, -0.5f), V2(1, 0) },
			{ V3(6, 0.03125f, -0.5f), V2(1, 0) },
			{ V3(6, -0.03125f, -0.5f), V2(0, 0) },

			//BackFace
			{ V3(-6, -0.03125f, 0.5f), V2(0, 0) },
			{ V3(-6, 0.03125f, 0.5f), V2(1, 0) },
			{ V3(6, 0.03125f, 0.5f), V2(1, 0) },
			{ V3(6, -0.03125f, 0.5f), V2(0, 0) }
		};

		local_global u32 ModelIndices[] = {
			//FrontFace
			0, 1, 2,
			2, 3, 0,
			//Backface
			6, 5, 4,
			4, 7, 6,
			//LeftFace
			4, 5, 1,
			1, 0, 4,
			//RightFace
			3, 2, 6,
			6 ,7 ,3,
			//TopFace
			1, 5, 6,
			6, 2, 1,
			//BottomFace
			4, 0, 3,
			3, 7, 4
		};

		local_global mesh Mesh = {};
		Mesh.TextureIndex = 0;

		Mesh.IndexOffset = 0;
		Mesh.IndexCount = ArrayCount(ModelIndices);

		Mesh.VertexOffset = 0;
		Mesh.VertexCount = ArrayCount(ModelVertices);

		GlobalState.border = {};

		GlobalState.border.NumMeshes = 1;
		GlobalState.border.MeshArray = &Mesh;

		GlobalState.border.VertexCount = ArrayCount(ModelVertices);
		GlobalState.border.VertexArray = ModelVertices;

		GlobalState.border.IndexCount = ArrayCount(ModelIndices);
		GlobalState.border.IndexArray = ModelIndices;

		GlobalState.border.NumTextures = 1;
		GlobalState.border.TextureArray = &Gradient;
	}
	// ArrowModel
	{
		
		local_global vertex ModelVertices[] = {
			//FrontFace
			{ V3(0.0f, 0.0f, -0.25f), V2(1, 1) },
			{ V3(-1.0f, 1.0f, -0.25f), V2(1, 1) },
			{ V3(-0.5f, 1.0f, -0.25f), V2(1, 1) },
			{ V3(-0.5f, 3.0f, -0.25f), V2(1, 1) },
			{ V3(0.5f, 3.0f, -0.25f), V2(1, 1) },
			{ V3(0.5f, 1.0f, -0.25f), V2(1, 1) },
			{ V3(1.0f, 1.0 , -0.25f), V2(1, 1) },

			//BackFace
			{ V3(0.0f, 0.0f, 0.25f), V2(0, 1) },
			{ V3(-1.0f, 1.0f, 0.25f), V2(0, 1) },
			{ V3(-0.5f, 1.0f, 0.25f), V2(0, 1) },
			{ V3(-0.5f, 3.0f, 0.25f), V2(0, 1) },
			{ V3(0.5f, 3.0f, 0.25f), V2(0, 1) },
			{ V3(0.5f, 1.0f, 0.25f), V2(0, 1) },
			{ V3(1.0f, 1.0f, 0.25f), V2(0, 1) },
		};

		local_global u32 ModelIndices[] = {
			//FrontFace
			0, 1, 2,
			0, 2, 5,
			0, 5, 6,
			2, 3, 5,
			5, 3, 4,
			//Backface
			7, 13, 12,
			7, 12, 9,
			7, 9, 8,
			9, 12, 11,
			9, 11, 10,
			//LeftFace
			2, 9, 10,
			2, 10, 3,
			//RightFace
			12, 5, 4,
			12, 4, 11,
			//TopFace
			4, 3, 10,
			4, 10, 11,
			2, 1, 8,
			2, 8, 9,
			6, 5, 12,
			6, 12, 13,
			//BottomFace
			0, 7, 8,
			0, 8, 1,
			7, 0, 6,
			7, 6, 13
		};

		local_global mesh Mesh = {};
		Mesh.TextureIndex = 0;

		Mesh.IndexOffset = 0;
		Mesh.IndexCount = ArrayCount(ModelIndices);

		Mesh.VertexOffset = 0;
		Mesh.VertexCount = ArrayCount(ModelVertices);

		GlobalState.Arrow = {};

		GlobalState.Arrow.NumMeshes = 1;
		GlobalState.Arrow.MeshArray = &Mesh;

		GlobalState.Arrow.VertexCount = ArrayCount(ModelVertices);
		GlobalState.Arrow.VertexArray = ModelVertices;

		GlobalState.Arrow.IndexCount = ArrayCount(ModelIndices);
		GlobalState.Arrow.IndexArray = ModelIndices;

		GlobalState.Arrow.NumTextures = 1;
		GlobalState.Arrow.TextureArray = &Gradient;
	}
	//MenuButtons
	{
		local_global vertex ModelVertices[] = {
			//FrontFace
			{ V3(-0.375f * 6, -0.375f, 0.5f), V2(0, 1) },
			{ V3(-0.375f * 6, 0.375f, 0.5f), V2(0, 0) },
			{ V3(0.375f * 6, 0.375f, 0.5f), V2(1, 0) },
			{ V3(0.375f * 6, -0.375f, 0.5f), V2(1, 1) }
		};

		local_global u32 ModelIndices[] =
		{
			//FrontFace
			0, 1, 2,
			2, 3, 0
		};

		GlobalState.Logo = {};
		GlobalState.Start = {};
		GlobalState.Options = {};
		GlobalState.Exit = {};

		GlobalState.Button0Logo = {};
		GlobalState.Button1Start = {};
		GlobalState.Button2Options = {};
		GlobalState.Button3Exit = {};

		GlobalState.Logo1 = {};
		GlobalState.Start2 = {};
		GlobalState.Options3 = {};
		GlobalState.Exit4 = {};

		model* currModel = &GlobalState.Logo;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &LogoText;
		}
		currModel = &GlobalState.Start;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &StartText;
		}
		currModel = &GlobalState.Options;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &OptionsText;
		}
		currModel = &GlobalState.Exit;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &ExitText;
		}

		modelInWorld* Object = &GlobalState.Button0Logo;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 0.0f;
			Object->PosY = 2.0f;
			Object->model = GlobalState.Logo;
		}
		Object = &GlobalState.Button1Start;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 0.0f;
			Object->PosY = 0.0f;
			Object->model = GlobalState.Start;
		}
		Object = &GlobalState.Button2Options;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 0.0f;
			Object->PosY = -1.0f;
			Object->model = GlobalState.Options;
		}
		Object = &GlobalState.Button3Exit;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 0.0f;
			Object->PosY = -1.0f;
			Object->model = GlobalState.Exit;
		}

		button* CurrButton = &GlobalState.Logo1;
		{
			CurrButton->Button = GlobalState.Button0Logo;
			CurrButton->isActive = FALSE;
		}
		CurrButton = &GlobalState.Start2;
		{
			CurrButton->Button = GlobalState.Button1Start;
			CurrButton->isActive = TRUE;
		}
		CurrButton = &GlobalState.Options3;
		{
			CurrButton->Button = GlobalState.Button2Options;
			CurrButton->isActive = FALSE;
		}
		CurrButton = &GlobalState.Exit4;
		{
			CurrButton->Button = GlobalState.Button3Exit;
			CurrButton->isActive = FALSE;
		}

		GlobalState.InterfaceButtons = (button*)malloc(sizeof(button) * 4);

		GlobalState.InterfaceButtons[0] = GlobalState.Logo1;
		GlobalState.InterfaceButtons[1] = GlobalState.Start2;
		//GlobalState.InterfaceButtons[2] = GlobalState.Options3;
		GlobalState.InterfaceButtons[2] = GlobalState.Exit4;

		GlobalState.currentButton = 1;
	}
	//ScoreNumbers
	{
		local_global vertex ModelVertices[] = {
			//FrontFace
			{ V3(-0.375f * 2, -0.375f * 3, 0.5f), V2(0, 1) },
			{ V3(-0.375f * 2, 0.375f * 3, 0.5f), V2(0, 0) },
			{ V3(0.375f * 2, 0.375f * 3, 0.5f), V2(1, 0) },
			{ V3(0.375f * 2, -0.375f * 3, 0.5f), V2(1, 1) }
		};

		local_global u32 ModelIndices[] = {
			//FrontFace
			0, 1, 2,
			2, 3, 0
		};

		GlobalState.LeftScoreMinorNum = {};
		GlobalState.LeftScoreIntermediateNum = {};
		GlobalState.LeftScoreMajorNum = {};
		
		GlobalState.RightScoreMinorNum = {};
		GlobalState.RightScoreIntermediateNum = {};
		GlobalState.RightScoreMajorNum = {};

		GlobalState.LeftScoreMinorNumModel = {};
		GlobalState.LeftScoreIntermediateNumModel = {};
		GlobalState.LeftScoreMajorNumModel = {};

		GlobalState.RightScoreMinorNumModel = {};
		GlobalState.RightScoreIntermediateNumModel = {};
		GlobalState.RightScoreMajorNumModel = {};

		model* currModel = &GlobalState.LeftScoreMinorNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}
		currModel = &GlobalState.LeftScoreIntermediateNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}
		currModel = &GlobalState.LeftScoreMajorNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}

		currModel = &GlobalState.RightScoreMinorNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}
		currModel = &GlobalState.RightScoreIntermediateNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}
		currModel = &GlobalState.RightScoreMajorNumModel;
		{
			local_global mesh Mesh = {};
			Mesh.TextureIndex = 0;

			Mesh.IndexOffset = 0;
			Mesh.IndexCount = ArrayCount(ModelIndices);

			Mesh.VertexOffset = 0;
			Mesh.VertexCount = ArrayCount(ModelVertices);

			currModel->NumMeshes = 1;
			currModel->MeshArray = &Mesh;

			currModel->VertexCount = ArrayCount(ModelVertices);
			currModel->VertexArray = ModelVertices;

			currModel->IndexCount = ArrayCount(ModelIndices);
			currModel->IndexArray = ModelIndices;

			currModel->NumTextures = 1;
			currModel->TextureArray = &NumZero;
		}


		modelInWorld* Object = &GlobalState.LeftScoreMinorNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = -2.0f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.LeftScoreMinorNumModel;
		}
		Object = &GlobalState.LeftScoreIntermediateNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = -3.5f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.LeftScoreIntermediateNumModel;
		}
		Object = &GlobalState.LeftScoreMajorNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = -5.0f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.LeftScoreMajorNumModel;
		}

		Object = &GlobalState.RightScoreMinorNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 5.0f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.RightScoreMinorNumModel;
		}
		Object = &GlobalState.RightScoreIntermediateNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 3.5f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.RightScoreIntermediateNumModel;
		}
		Object = &GlobalState.RightScoreMajorNum;
		{
			Object->Velocity = V3(0.0f, 0.0f, 0.0f);
			Object->PosX = 2.0f;
			Object->PosY = 1.5f;
			Object->model = GlobalState.RightScoreMajorNumModel;
		}
	}
	//BallModel
	{
		//Vertices
		
		// x^2+y^2+z^2=1
		// diameter = 0.03125
		// radius = 0.015625
		local_global std::vector<vertex> ModelVertices;
		// low point = (0.0, -d/2, 0.0)
		ModelVertices.push_back({V3(0.0f, -BallDiameter/2, 0.0f), V2(0.5f, 1.0f)});
		// for Y
		// For Slice of circle 
		for (f32 y = 1; y < BallQuality; y++)
		{

			f32 height = BallDiameter / BallQuality * y;
			if (height < BallDiameter / 2)
			{
				height = BallDiameter / 2 - height;
			}
			else
			{
				height -= BallDiameter / 2;
			}

			f32 currentRadius = sqrt((BallDiameter / 2 * BallDiameter / 2) - (height * height));
			// x^2+z^2=currentRadius^2

			height = BallDiameter / BallQuality * y;
			for (f32 angle = 0; angle < BallQuality; angle ++)
			{
				f32 currentAngle = angle / BallQuality * 2 * 3.14159f;
				ModelVertices.push_back({ V3(cosf(currentAngle) * currentRadius, -BallDiameter/2 + height, sinf(currentAngle) * currentRadius), V2(0.5f, 1.0f)});
			}
		}
		// high point = (0.0, d/2, 0.0)
		ModelVertices.push_back({ V3(0.0f, BallDiameter/2, 0.0f), V2(0.5f, 1.0f) });

		//Indices
		local_global std::vector<u32> ModelIndices;
		for (f32 angle = 0; angle < BallQuality; angle++)
		{
			u32 currBigAngleID = 1 + angle;
			ModelIndices.push_back(0);
			
			if(currBigAngleID - 1 == 0)
				ModelIndices.push_back(BallQuality);
			else
				ModelIndices.push_back(currBigAngleID - 1);
			ModelIndices.push_back(currBigAngleID);
		}
		for (f32 ñurrentBigHeightLvl = 2; ñurrentBigHeightLvl < BallQuality; ñurrentBigHeightLvl++)
		{
			for (f32 angle = 0; angle < BallQuality; angle++)
			{
				u32 currBigAngleID = 1 + angle;


				if (currBigAngleID - 1 == 0)
				{
					ModelIndices.push_back((ñurrentBigHeightLvl - 1) * BallQuality);
					ModelIndices.push_back((ñurrentBigHeightLvl) * BallQuality);
					ModelIndices.push_back((ñurrentBigHeightLvl - 2) * BallQuality + currBigAngleID);
					
					ModelIndices.push_back((ñurrentBigHeightLvl - 2) * BallQuality + currBigAngleID);
					ModelIndices.push_back((ñurrentBigHeightLvl) * BallQuality);
					ModelIndices.push_back((ñurrentBigHeightLvl - 1)* BallQuality + currBigAngleID);
				}
				else
				{
					ModelIndices.push_back((ñurrentBigHeightLvl -2) * BallQuality + currBigAngleID - 1);
					ModelIndices.push_back((ñurrentBigHeightLvl - 1) * BallQuality + currBigAngleID - 1);
					ModelIndices.push_back((ñurrentBigHeightLvl - 2) * BallQuality + currBigAngleID);


					ModelIndices.push_back((ñurrentBigHeightLvl - 2)*BallQuality + currBigAngleID);
					ModelIndices.push_back((ñurrentBigHeightLvl - 1)* BallQuality + currBigAngleID - 1);
					ModelIndices.push_back((ñurrentBigHeightLvl - 1)*BallQuality + currBigAngleID);
				}
			}
		}
		for (f32 angle = 0; angle < BallQuality; angle++)
		{
			u32 currBigAngleID = 1 + angle;
			ModelIndices.push_back((BallQuality - 1) * BallQuality + 1);

			ModelIndices.push_back((BallQuality - 2)* BallQuality + currBigAngleID);

			if (currBigAngleID - 1 == 0)
				ModelIndices.push_back((BallQuality - 1) * BallQuality);
			else
				ModelIndices.push_back((BallQuality - 2)* BallQuality + currBigAngleID - 1);
		}


		local_global mesh Mesh = {};
		Mesh.TextureIndex = 0;

		Mesh.IndexOffset = 0;
		Mesh.IndexCount = ModelIndices.size();

		Mesh.VertexOffset = 0;
		Mesh.VertexCount = ModelVertices.size();

		GlobalState.sphere = {};

		GlobalState.sphere.NumMeshes = 1;
		GlobalState.sphere.MeshArray = &Mesh;

		GlobalState.sphere.VertexCount = ModelVertices.size();
		GlobalState.sphere.VertexArray = ModelVertices.data();

		GlobalState.sphere.IndexCount = ModelIndices.size();
		GlobalState.sphere.IndexArray = ModelIndices.data();

		GlobalState.sphere.NumTextures = 1;
		GlobalState.sphere.TextureArray = &Gradient;

	}

	//Main loop and its variables
	LARGE_INTEGER beginTime = {};
	LARGE_INTEGER endTime = {};
	Assert(QueryPerformanceCounter(&beginTime));

	//Funny variable to use camera zooming
	{
		GlobalState.Camera.CurrRotationCenterZCoordinate = 4.0f;
	}

	//Main loop
	while (GlobalState.IsRunning)
	{
		Assert(QueryPerformanceCounter(&endTime));
		f32 frameTime = f32(endTime.QuadPart - beginTime.QuadPart) / f32(timerFrequency.QuadPart);
		beginTime = endTime;

		//Debuging FPS
		{
			char Text[256];
			snprintf(Text, sizeof(Text), "FrameTime: %f\n", frameTime);
			OutputDebugStringA(Text);
		}

		//Input handling
		MSG Message = {};
		while (PeekMessageA(&Message, GlobalState.WindowHandle, 0, 0, PM_REMOVE))
		{
			u32 VkCode = Message.wParam;
			switch (Message.message)
			{
			case WM_QUIT:
			{ GlobalState.IsRunning = false;
			} break;
			case WM_KEYUP:
			{
				b32 IsUp = (Message.lParam >> 31) & 0x1;
				switch (VkCode)
				{
					case VK_UP:
					{
						GlobalState.UpKeyCanPush = true;

					} break;
					case VK_DOWN:
					{
						GlobalState.DownKeyCanPush = true;
					} break;
				}
			}
			case WM_KEYDOWN:
			{
				b32 IsDown = !(Message.lParam >> 31) & 0x1;
				switch (VkCode) 
				{
					case 'W':
					{
						GlobalState.WDown = IsDown;
					} break;
					case 'A':
					{
						GlobalState.ADown = IsDown;
					} break;
					case 'S':
					{
						GlobalState.SDown = IsDown;
					} break;
					case 'D':
					{
						GlobalState.DDown = IsDown;
					} break;
					case VK_UP:
					{
						GlobalState.UpKeyDown = IsDown;
						GlobalState.currentButton -= 1 * !(GlobalState.currentButton == 1) * !(GlobalState.currentButton == 4)
							* !GlobalState.GameStart * GlobalState.UpKeyCanPush;
						GlobalState.UpKeyCanPush = false;
					} break;
					case VK_DOWN:
					{
						GlobalState.DownKeyDown = IsDown;
						GlobalState.currentButton += 1 * !(GlobalState.currentButton == 2) * !(GlobalState.currentButton == 7)
							* !GlobalState.GameStart * GlobalState.DownKeyCanPush;
						GlobalState.DownKeyCanPush = false;
					} break;

					case 'J':
					{
						GlobalState.Camera.Pos = V3(0, 0, 0);
						GlobalState.Camera.Pitch = 0;
						GlobalState.Camera.Yaw = 0;
					} break;

					case VK_RETURN:
					{
						GlobalState.EnterKeyDown = IsDown;
					} break;

					case VK_ESCAPE:
					{
						GlobalState.IsRunning = false;
					} break;
				}
			} break;

			default:
			{ 
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
			}
		}

	
		if (GlobalState.EnterKeyDown)
		{
			GlobalState.MatchStart = true * (GlobalState.GameStart);
			
			//Pressing a button in menu
			switch (GlobalState.currentButton)
			{
				case 1:
				{
					GlobalState.GameStart = true;
				} break;
				case 2:
				{
					GlobalState.IsRunning = false;
				} break;
			}
		}

		//Clearing screen
		for (u32 Y = 0; Y < GlobalState.FrameBufferHeight; ++Y)
		{
			for (u32 X = 0; X < GlobalState.FrameBufferWidth; ++X)
			{
				u32 pixelID = (Y * GlobalState.FrameBufferStride) + X;
				GlobalState.FrameBufferPixels[pixelID] = 0xFF000000; // dark background
				//GlobalState.FrameBufferPixels[pixelID] = 0xFFFFFFFF; // white background
				GlobalState.DepthBuffer[pixelID] = FLT_MAX;
			}
		}

		f32 rotationSpeed = f32(1.0f);
		GlobalState.CurrRot += frameTime * rotationSpeed;
		if (GlobalState.CurrRot > 2.0f * 3.14159f)
		{
			GlobalState.CurrRot -= 2.0f * 3.14159f;
		}

		RECT clientRect = {};
		Assert(GetClientRect(GlobalState.WindowHandle, &clientRect));
		u32 clientWidth = clientRect.right - clientRect.left;
		u32 clientHeight = clientRect.bottom - clientRect.top;

		f32 AspectRatio = f32(clientWidth) / f32(clientHeight);

		//Creating matrix for camera
		m4 CameraTransform = IdentityM4();
		{
			camera* Camera = &GlobalState.Camera;

			b32 MouseDown = false;
			v2 CurrMousePos = {};
			if(GetActiveWindow() == GlobalState.WindowHandle)
			{
				POINT Win32MousePos = {};
				Assert(GetCursorPos(&Win32MousePos));
				Assert(ScreenToClient(GlobalState.WindowHandle, &Win32MousePos));

				Win32MousePos.y = clientRect.bottom - Win32MousePos.y;

				CurrMousePos.x = f32(Win32MousePos.x) / f32(clientWidth);
				CurrMousePos.y = f32(Win32MousePos.y) / f32(clientHeight);

				MouseDown = (GetKeyState(VK_LBUTTON) & 0x80) != 0;

			}
			
			if(MouseDown)
			{
				Camera->YawVelocity = 0;
				Camera->PitchVelocity = 0;
				if (!Camera->PrevMouseDown)
				{
					Camera->PrevMousePos = CurrMousePos;
				}

				v2 MouseDelta = CurrMousePos - Camera->PrevMousePos;
				
				
				Camera->Yaw -= MouseDelta.x;
				Camera->Pitch -= MouseDelta.y;

				Camera->PrevMousePos = CurrMousePos;
			}
			else if (Camera->PrevMouseDown)
			{
				v2 MouseDelta = CurrMousePos - Camera->PrevMousePos;
				Camera->YawVelocity = -MouseDelta.x * 20;
				Camera->PitchVelocity = -MouseDelta.y * 20;
			}

			Camera->PrevMouseDown = MouseDown;

			Camera->Yaw += Camera->YawVelocity * frameTime;
			Camera->Pitch += Camera->PitchVelocity * frameTime;

			Camera->YawVelocity *= 0.95f;
			Camera->PitchVelocity *= 0.95f;

			Camera->YawVelocity *= (abs(Camera->YawVelocity) > 0.1);
			Camera->PitchVelocity *= (abs(Camera->PitchVelocity) > 0.1);

			m4 YawTransform = RotationMatrix(0, Camera->Yaw, 0);
			m4 PitchTransform = RotationMatrix(Camera->Pitch, 0, 0);
			m4 CameraAxisTransform = YawTransform * PitchTransform;
			
			v3 Right = Normalize((CameraAxisTransform * V4(1, 0, 0, 0)).xyz);
			v3 Up = Normalize((CameraAxisTransform * V4(0, 1, 0, 0)).xyz);
			v3 LookAt = Normalize((CameraAxisTransform * V4(0, 0, 1, 0)).xyz);

			m4 CameraViewTransform = IdentityM4();

			CameraViewTransform.v[0].x = Right.x;
			CameraViewTransform.v[1].x = Right.y;
			CameraViewTransform.v[2].x = Right.z;

			CameraViewTransform.v[0].y = Up.x;
			CameraViewTransform.v[1].y = Up.y;
			CameraViewTransform.v[2].y = Up.z;

			CameraViewTransform.v[0].z = LookAt.x;
			CameraViewTransform.v[1].z = LookAt.y;
			CameraViewTransform.v[2].z = LookAt.z;

			Camera->Pos.x = sinf(Camera->Yaw) * cosf(Camera->Pitch);
			Camera->Pos.x *= GlobalState.Camera.CurrRotationCenterZCoordinate;
			Camera->Pos.y = sinf(Camera->Pitch);
			Camera->Pos.y *= GlobalState.Camera.CurrRotationCenterZCoordinate;
			Camera->Pos.z = 1 - (cosf(Camera->Yaw) * cosf(Camera->Pitch));
			Camera->Pos.z *= GlobalState.Camera.CurrRotationCenterZCoordinate;

			if (GlobalState.WDown)
			{
				GlobalState.Camera.CurrRotationCenterZCoordinate -= 1 * frameTime * (GlobalState.Camera.CurrRotationCenterZCoordinate > 0.0f);
			}
			if (GlobalState.SDown)
			{
				GlobalState.Camera.CurrRotationCenterZCoordinate += 1 * frameTime;
			}

			CameraTransform = CameraViewTransform * TranslationMatrix(-Camera->Pos);
		}
		

		//Moving Rectangles
		//Left
		{
			modelInWorld* object = &GlobalState.LeftRectangle;
			if (GlobalState.UpKeyDown)
			{
				if (object->Velocity.y < 0)
				{
					object->Velocity.y += 8.5 * frameTime;
				}
				else
				{
					object->Velocity.y += 4.5 * frameTime;
				}

				if (object->Velocity.y > 10.0)
					object->Velocity.y = 10.0f;
			}
			else if (GlobalState.DownKeyDown)
			{
				if (object->Velocity.y > 0)
				{
					object->Velocity.y -= 8.5 * frameTime;
				}
				else
				{
					object->Velocity.y -= 4.5 * frameTime;
				}
				
				if (object->Velocity.y < -10.0)
					object->Velocity.y = -10.0f;
			}
			else
			{
				if (object->Velocity.y < 0.0f)
				object->Velocity.y += 6.5 * frameTime;

				if (object->Velocity.y > 0.0f)
					object->Velocity.y -= 2.5 * frameTime;
			}


			object->PosY += object->Velocity.y * frameTime;
			if (object->PosY > 2.5 - 0.03125f * DifficultySize)
			{
				object->PosY = 2.5f - 0.03125f * DifficultySize;
				object->Velocity.y = 0.0f;
			}
			else if (object->PosY < -2.5 + 0.03125f * DifficultySize)
			{
				object->PosY = -2.5f + 0.03125f * DifficultySize;
				object->Velocity.y = 0.0f;
			}
		}
		//Right Ractangle AI
		{
			if (GlobalState.Ball.PosY > GlobalState.RightRectangle.PosY && GlobalState.Ball.Velocity.x > 0.0f)
			{
				GlobalState.ShouldGoUp = true;
				GlobalState.ShouldGoDown = false;
			}
			else if (GlobalState.Ball.PosY < GlobalState.RightRectangle.PosY && GlobalState.Ball.Velocity.x > 0.0f)
			{
				GlobalState.ShouldGoDown = true;
				GlobalState.ShouldGoUp = false;
			}
			else
			{
				if (GlobalState.RightRectangle.Velocity.y < 0)
				{
					GlobalState.RightRectangle.Velocity.y += 8.5 * frameTime;
				}
				else
				{
					GlobalState.RightRectangle.Velocity.y -= 8.5 * frameTime;
				}
			}
		}
		//Right
		{
			modelInWorld* object = &GlobalState.RightRectangle;
			if (GlobalState.ShouldGoUp)
			{
				if (object->Velocity.y < 0)
				{
					object->Velocity.y += 8.5 * frameTime;
				}
				else
				{
					object->Velocity.y += 4.5 * frameTime;
				}
				if (object->Velocity.y > 10.0)
					object->Velocity.y = 10.0f;
			}
			else if (GlobalState.ShouldGoDown)
			{
				if (object->Velocity.y > 0)
				{
					object->Velocity.y -= 8.5 * frameTime;
				}
				else
				{
					object->Velocity.y -= 4.5 * frameTime;
				}
				if (object->Velocity.y < -10.0)
					object->Velocity.y = -10.0f;
			}
			else
			{
				if (object->Velocity.y < 0.0f)
					object->Velocity.y += 6.5 * frameTime;

				if (object->Velocity.y > 0.0f)
					object->Velocity.y -= 2.5 * frameTime;
			}


			object->PosY += object->Velocity.y * frameTime;
			if (object->PosY > 2.5 - 0.03125f * DifficultySize)
			{
				object->PosY = 2.5f - 0.03125f * DifficultySize;
				object->Velocity.y = 0.0f;
			}
			else if (object->PosY < -2.5 + 0.03125f * DifficultySize)
			{
				object->PosY = -2.5f + 0.03125f * DifficultySize;
				object->Velocity.y = 0.0f;
			}
		}


		//Moving ball and checking his collision
		srand(beginTime.LowPart);
		{
			modelInWorld* object = &GlobalState.Ball;
			f32 angle{};
			if (!GlobalState.MatchStart)
			{
				angle = rand();// / RAND_MAX;
				object->Velocity.xy = V2(cosf(angle) * BaseSpeed, sinf(angle) * BaseSpeed);
			}

			else {

				//handling bug when x velocity of ball is near zero
				if (object->Velocity.x > -BaseSpeed * 0.5f && object->Velocity.x < BaseSpeed * 0.5f)
				{
					object->Velocity.x *= 1.1;

					// if velocity is zero we add it a little in random direction
					object->Velocity.x += (object->Velocity.x == 0) * ((2 * angle - RAND_MAX) / RAND_MAX) * 0.1;
				}

				b32 CollissionDetected = false;
				modelInWorld* collisionObject = {};

				object->PosY += object->Velocity.y * frameTime;
				object->PosX += object->Velocity.x * frameTime;
				if (object->PosY > 2.5 - BallDiameter / 2)
				{
					object->PosY = 2.5f - BallDiameter / 2;
					object->Velocity.y *= -1.0f;
				}
				else if (object->PosY < -2.5 + BallDiameter / 2)
				{
					object->PosY = -2.5f + BallDiameter / 2;
					object->Velocity.y *= -1.0f;
				}

				if (object->PosX + BallDiameter / 2 > 4 - 0.03125f 
					&& object->PosX - BallDiameter / 2 < 4 + 0.03125f)
				{
					collisionObject = &GlobalState.RightRectangle;
					if (object->PosY + BallDiameter / 2 > collisionObject->PosY - DifficultySize * 0.03125f
						&& object->PosY - BallDiameter / 2 < collisionObject->PosY + DifficultySize * 0.03125f)
					{
						CollissionDetected = true;
						object->PosX = 3.99 - BallDiameter / 2;
					}
				}
				else if (object->PosX - BallDiameter / 2 < -4 + 0.03125f 
					&& object->PosX + BallDiameter / 2 > -4 - 0.03125f)
				{
					collisionObject = &GlobalState.LeftRectangle;
					if (object->PosY + BallDiameter / 2 > collisionObject->PosY - DifficultySize * 0.03125f
						&& object->PosY - BallDiameter / 2 < collisionObject->PosY + DifficultySize * 0.03125f)
					{
						CollissionDetected = true;
						object->PosX = -3.99 + BallDiameter / 2;
					}
				}

				if (CollissionDetected)
				{
					object->Velocity.y += collisionObject->Velocity.y;

					f32 collisionObjectVelocityValue = abs(collisionObject->Velocity.y);
					f32 objectVelocityValue = sqrt(object->Velocity.x * object->Velocity.x + object->Velocity.y * object->Velocity.y);

					object->Velocity.x /= objectVelocityValue;
					object->Velocity.y /= objectVelocityValue;
					object->Velocity.x *= BaseSpeed + collisionObjectVelocityValue;
					object->Velocity.y *= BaseSpeed + collisionObjectVelocityValue;

					object->Velocity.x *= -1.0f;
				}
			}
		}
		//Checking ball position for score update
		{
			modelInWorld* object = &GlobalState.Ball;

			if (object->PosX - BallDiameter / 2 < -6)
			{
				GlobalState.MatchStart = FALSE;
				GlobalState.GameScore.matchesPlayed += 1;
				GlobalState.GameScore.scoreRightPlayer += 1;


				GlobalState.RightScoreMajorNum.model.TextureArray = NumbersTextures[(GlobalState.GameScore.scoreRightPlayer / 100) % 10];
				GlobalState.RightScoreIntermediateNum.model.TextureArray = NumbersTextures[(GlobalState.GameScore.scoreRightPlayer / 10) % 10];
				GlobalState.RightScoreMinorNum.model.TextureArray = NumbersTextures[GlobalState.GameScore.scoreRightPlayer % 10];


				object->PosX = 0;
				object->PosY = 0;
				object = &GlobalState.LeftRectangle;
				object->PosY = 0;
				object->Velocity.y = 0;
				object = &GlobalState.RightRectangle;
				object->PosY = 0;
				object->Velocity.y = 0;

			}
			else if (object->PosX + BallDiameter / 2 > 6)
			{
				GlobalState.MatchStart = FALSE;
				GlobalState.GameScore.matchesPlayed += 1;
				GlobalState.GameScore.scoreLeftPlayer += 1;


				GlobalState.LeftScoreMajorNum.model.TextureArray = NumbersTextures[(GlobalState.GameScore.scoreLeftPlayer / 100) % 10];
				GlobalState.LeftScoreIntermediateNum.model.TextureArray = NumbersTextures[(GlobalState.GameScore.scoreLeftPlayer / 10) % 10];
				GlobalState.LeftScoreMinorNum.model.TextureArray = NumbersTextures[GlobalState.GameScore.scoreLeftPlayer % 10];


				object->PosX = 0;
				object->PosY = 0;
				object = &GlobalState.LeftRectangle;
				object->PosY = 0;
				object->Velocity.y = 0;
				object = &GlobalState.RightRectangle;
				object->PosY = 0;
				object->Velocity.y = 0;
			}
		}

		// Drawing Interface
		Sampler.Type = SamplerType_Bilinear;
		if (!GlobalState.GameStart)
		{

			for (u32 i = 0; i < 3; i++)
			{
				m4 TransformButton = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
					* CameraTransform
					* TranslationMatrix(GlobalState.InterfaceButtons[i].Button.PosX, GlobalState.InterfaceButtons[i].Button.PosY, 2.5f / 4.0f * GlobalState.Camera.CurrRotationCenterZCoordinate)
					* RotationMatrix(0, 0, 0)
					* ScaleMatrix(1, 1, 1);

				DrawModel(&GlobalState.InterfaceButtons[i].Button.model, TransformButton, Sampler);
			}

			Sampler.Type = SamplerType_Nearest;
			m4 TransformArrow = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* CameraTransform
				* TranslationMatrix(0.375f * 6 + (sinf(GlobalState.CurrRot * 2.0f) + 1.0f), GlobalState.InterfaceButtons[GlobalState.currentButton].Button.PosY, 3.0f / 4.0f * GlobalState.Camera.CurrRotationCenterZCoordinate)
				* RotationMatrix(0.0f, GlobalState.CurrRot, -3.14159f / 2.0f)
				* ScaleMatrix(0.5f, 0.5f, 0.5f);

			DrawModel(&GlobalState.Arrow, TransformArrow, Sampler);

		}
		// Drawing Score (Game is started)
		else
		{
			u32	scoreDistance = 16;

			m4 TransformLeftScoreMajorNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.LeftScoreMajorNum.PosX, scoreDistance * GlobalState.LeftScoreMajorNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			m4 TransformLeftScoreIntermediateNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.LeftScoreIntermediateNum.PosX, scoreDistance * GlobalState.LeftScoreIntermediateNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			m4 TransformLeftScoreMinorNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.LeftScoreMinorNum.PosX, scoreDistance * GlobalState.LeftScoreMinorNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			m4 TransformRightScoreMajorNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.RightScoreMajorNum.PosX, scoreDistance * GlobalState.RightScoreMajorNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			m4 TransformRightScoreIntermediateNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.RightScoreIntermediateNum.PosX, scoreDistance * GlobalState.RightScoreIntermediateNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			m4 TransformRightScoreMinorNum = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
				* TranslationMatrix(scoreDistance * GlobalState.RightScoreMinorNum.PosX, scoreDistance * GlobalState.RightScoreMinorNum.PosY, scoreDistance * 4)
				* RotationMatrix(0, 0, 0)
				* ScaleMatrix(scoreDistance, scoreDistance, scoreDistance);

			DrawModel(&GlobalState.LeftScoreMajorNum.model, TransformLeftScoreMajorNum, Sampler);
			DrawModel(&GlobalState.LeftScoreIntermediateNum.model, TransformLeftScoreIntermediateNum, Sampler);
			DrawModel(&GlobalState.LeftScoreMinorNum.model, TransformLeftScoreMinorNum, Sampler);

			DrawModel(&GlobalState.RightScoreMajorNum.model, TransformRightScoreMajorNum, Sampler);
			DrawModel(&GlobalState.RightScoreIntermediateNum.model, TransformRightScoreIntermediateNum, Sampler);
			DrawModel(&GlobalState.RightScoreMinorNum.model, TransformRightScoreMinorNum, Sampler);
		}

		//Drawing Game Objects
		f32 Offset = abs(sin(GlobalState.CurrRot));

		m4 TransformLeftRectangle = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
			* CameraTransform
			* TranslationMatrix(-4, GlobalState.LeftRectangle.PosY, GlobalState.Camera.CurrRotationCenterZCoordinate)
			* RotationMatrix(0, 0, 0)
			* ScaleMatrix(1, 1, 1);

		m4 TransformRightRectangle = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
			* CameraTransform
			* TranslationMatrix(4, GlobalState.RightRectangle.PosY, GlobalState.Camera.CurrRotationCenterZCoordinate)
			* RotationMatrix(0, 3.14159f, 0)
			* ScaleMatrix(1, 1, 1);

		m4 TransformTopBorder = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
			* CameraTransform
			* TranslationMatrix(0, 2.5f, GlobalState.Camera.CurrRotationCenterZCoordinate)
			* RotationMatrix(3.14159f, 0, 0)
			* ScaleMatrix(1, 1, 1);

		m4 TransformBottomBorder = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
			* CameraTransform
			* TranslationMatrix(0, -2.5f, GlobalState.Camera.CurrRotationCenterZCoordinate)
			* RotationMatrix(0, 0, 0)
			* ScaleMatrix(1, 1, 1);

		m4 Transformball = PerspectiveMatrix(90.0f, AspectRatio, 0.01f, 1000.0f)
			* CameraTransform
			* TranslationMatrix(GlobalState.Ball.PosX, GlobalState.Ball.PosY, GlobalState.Camera.CurrRotationCenterZCoordinate)
			* RotationMatrix(0, 0, 0)
			* ScaleMatrix(1, 1, 1);

		Sampler.Type = SamplerType_Nearest;

		DrawModel(&GlobalState.rectangle, TransformLeftRectangle, Sampler);
		DrawModel(&GlobalState.rectangle, TransformRightRectangle, Sampler);

		DrawModel(&GlobalState.border, TransformTopBorder, Sampler);
		DrawModel(&GlobalState.border, TransformBottomBorder, Sampler);
		
		DrawModel(&GlobalState.sphere, Transformball, Sampler);
		
		//Creating image and drawing it from buffer to window
		BITMAPINFO BitmapInfo = {};
		BitmapInfo.bmiHeader = {};
		BitmapInfo.bmiHeader.biSize = sizeof(tagBITMAPINFOHEADER);
		BitmapInfo.bmiHeader.biWidth = GlobalState.FrameBufferStride;
		BitmapInfo.bmiHeader.biHeight = GlobalState.FrameBufferHeight;
		BitmapInfo.bmiHeader.biPlanes = 1;
		BitmapInfo.bmiHeader.biBitCount = 32;
		BitmapInfo.bmiHeader.biCompression = BI_RGB;

		Assert(StretchDIBits(
			GlobalState.DeviceContext,
			0,
			0,
			clientWidth,
			clientHeight,
			0,
			0,
			GlobalState.FrameBufferWidth,
			GlobalState.FrameBufferHeight,
			GlobalState.FrameBufferPixels,
			&BitmapInfo,
			DIB_RGB_COLORS,
			SRCCOPY));
	}
	return 0;
}