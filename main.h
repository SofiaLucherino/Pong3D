#if !defined(WIN32_GRAPHICSPRACTISE_H)

#include <cstdlib>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <float.h>
#include <cmath>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t mm;
typedef uintptr_t umm;

typedef int32_t b32;

#define global static
#define local_global static

#define snprintf _snprintf_s
#define Assert(Expression) if (!(Expression)) {__debugbreak();}
#define InvalidCodePath Assert(!"Invalid Code Path")
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define KiloBytes(Val) ((Val)*1024LL)
#define MegaBytes(Val) (KiloBytes(Val)*1024LL)
#define GigaBytes(Val) (MegaBytes(Val)*1024LL)
#define TeraBytes(Val) (GigaBytes(Val)*1024LL)

#include "graphicsMath.h"
#include "clipper.h"
#include "assets.h"


enum sampler_type 
{
    SamplerType_None,

    SamplerType_Nearest,
    SamplerType_Bilinear,
};

struct sampler 
{
    sampler_type Type;
    u32 BoarderColor;
};

struct camera
{
    b32 PrevMouseDown;
    v2 PrevMousePos;

    f32 Yaw;
    f32 YawVelocity;
    f32 Pitch;
    f32 PitchVelocity;
    f32 CurrRotationCenterZCoordinate;
    v3 Pos;

};

struct score 
{
    u32 scoreLeftPlayer;
    u32 scoreRightPlayer;
    
    u32 matchesPlayed;
};

struct modelInWorld 
{
    f32 PosY;
    f32 PosX;
    v3 Velocity;

    f32 Yaw; 
    f32 Pitch;

    model model;
};

struct button
{
    modelInWorld Button;
    b32 isActive;
};

struct global_state
{
    b32 IsRunning;
    HWND WindowHandle;
    HDC DeviceContext;
    u32 FrameBufferWidth;
    u32 FrameBufferStride;
    u32 FrameBufferHeight;
    u32* FrameBufferPixels;
    f32* DepthBuffer;

    f32 CurrTime;
    f32 CurrRot;

    b32 WDown;
    b32 ADown;
    b32 SDown;
    b32 DDown;

    b32 UpKeyDown;
    b32 DownKeyDown;
    b32 UpKeyCanPush;
    b32 DownKeyCanPush;

    b32 ShouldGoUp;
    b32 ShouldGoDown;

    b32 EnterKeyDown;
    b32 MatchStart;
    b32 GameStart;

    button* InterfaceButtons;
    u32 currentButton;

    camera Camera;

    model sphere;
    modelInWorld Ball;

    model rectangle;
    modelInWorld LeftRectangle;
    modelInWorld RightRectangle;

    model border;
    modelInWorld TopBorder;
    modelInWorld BottomBorder;


    model Logo;
    model Start;
    model Options;
    model Exit;

    modelInWorld Button0Logo;
    modelInWorld Button1Start;
    modelInWorld Button2Options;
    modelInWorld Button3Exit;

    modelInWorld ButtonScoreLeft;
    modelInWorld ButtonScoreRight;

    button Logo1;
    button Start2;
    button Options3;
    button Exit4;

    score GameScore;

    model LeftScoreMinorNumModel;
    model LeftScoreIntermediateNumModel;
    model LeftScoreMajorNumModel;

    model RightScoreMinorNumModel;
    model RightScoreIntermediateNumModel;
    model RightScoreMajorNumModel;

    modelInWorld LeftScoreMinorNum;
    modelInWorld LeftScoreIntermediateNum;
    modelInWorld LeftScoreMajorNum;

    modelInWorld RightScoreMinorNum;
    modelInWorld RightScoreIntermediateNum;
    modelInWorld RightScoreMajorNum;

    model Arrow;
    modelInWorld MenuArrow;
};

#define WIN32_GRAPHICSPRACTISE_H
#endif
