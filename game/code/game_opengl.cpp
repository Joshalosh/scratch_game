
#include "game_render_group.h"

#define GL_FRAMEBUFFER_SRGB                       0x8DB9
#define GL_SRGB8_ALPHA8                           0x8C43

#define GL_SHADING_LANGUAGE_VERSION               0x8B8C

#define GL_FRAMEBUFFER                            0x8D40
#define GL_COLOR_ATTACHMENT0                      0x8CE0
#define GL_FRAMEBUFFER_COMPLETE                   0x8CD5

// NOTE: Windows specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

struct opengl_info
{
    b32 ModernContext;

    char *Vendor;
    char *Renderer;
    char *Version;
    char *ShadingLanguageVersion;
    char *Extensions;

    b32 GL_EXT_texture_sRGB;
    b32 GL_EXT_framebuffer_sRGB;
    b32 GL_ARB_framebuffer_object;
};

internal s32
OpenGLParseNumber(char *At)
{
    s32 Result = 0;

    while((*At >= '0') && (*At <= '9'))
    {
        Result *= 10;
        Result += (*At - '0');
        ++At;
    }

    return(Result);
}

internal opengl_info
OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result = {};

    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)glGetString(GL_VENDOR);
    Result.Renderer = (char *)glGetString(GL_RENDERER);
    Result.Version = (char *)glGetString(GL_VERSION);
    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        Result.ShadingLanguageVersion = "(none)";
    }

    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);

    char *At = Result.Extensions;
    while(*At)
    {
        while(IsWhitespace(*At)) {++At;}
        char *End = At;
        while(*End && !IsWhitespace(*End)) {++End;}

        umm Count = End - At;

        if(0) {}
        else if(StringsAreEqual(Count, At, "GL_EXT_texture_sRGB")) {Result.GL_EXT_texture_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_EXT_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_ARB_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_ARB_framebuffer_object")) {Result.GL_ARB_framebuffer_object=true;}

        At = End;
    }

    char *MajorAt = Result.Version;
    char *MinorAt = 0;
    for(char *At = Result.Version; *At; ++At)
    {
        if(At[0] == '.')
        {
            MinorAt = At + 1;
            break;
        }
    }

    s32 Major = 1;
    s32 Minor = 0;
    if(MinorAt)
    {
        Major = OpenGLParseNumber(MajorAt);
        Minor = OpenGLParseNumber(MinorAt);
    }

    if((Major > 2) || ((Major == 2) && (Minor >= 1)))
    {
        // NOTE: I believe there are sRGB textures in 2.1 and above automatically
        Result.GL_EXT_texture_sRGB = true;
    }

    return(Result);
}

internal opengl_info
OpenGLInit(b32 ModernContext, b32 FramebufferSupportsSRGB)
{
    opengl_info Info = OpenGLGetInfo(ModernContext);

    // NOTE: If I can do fill sRGB on the texture side 
    // and the frame buffer side, then I can enable it, otherwise it is 
    // safer to just pass it straight through
    OpenGLDefaultInternalTextureFormat = GL_RGBA8;
    if(FramebufferSupportsSRGB && Info.GL_EXT_texture_sRGB && Info.GL_EXT_framebuffer_sRGB)
    {
        OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    return(Info);
}

global_variable u32 GlobalFramebufferCount = 1;
global_variable GLuint GlobalFramebufferHandles[256] = {0};
global_variable GLuint GlobalFramebufferTextures[256] = {0};
internal void
OpenGLBindFramebuffer(u32 TargetIndex, rectangle2i DrawRegion)
{
    u32 WindowWidth = GetWidth(DrawRegion);
    u32 WindowHeight = GetHeight(DrawRegion);

    glBindFramebuffer(GL_FRAMEBUFFER, GlobalFramebufferHandles[TargetIndex]);
    if(TargetIndex > 0)
    {
        glViewport(0, 0, WindowWidth, WindowHeight);
    }
    else
    {
        glViewport(DrawRegion.MinX, DrawRegion.MinY, WindowWidth, WindowHeight);
    }
}

inline void
OpenGLSetScreenspace(s32 Width, s32 Height)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    r32 a = SafeRatio1(2.0f, (r32)Width);
    r32 b = SafeRatio1(2.0f, (r32)Height);
    r32 Proj[] =
    {
         a,  0,  0,  0,
         0,  b,  0,  0,
         0,  0,  1,  0,
        -1, -1,  0,  1,
    };
    glLoadMatrixf(Proj);
}

inline void
OpenGLLineVertices(v2 MinP, v2 MaxP)
{
    glVertex2f(MinP.x, MinP.y);
    glVertex2f(MaxP.x, MinP.y);

    glVertex2f(MaxP.x, MinP.y);
    glVertex2f(MaxP.x, MaxP.y);

    glVertex2f(MaxP.x, MaxP.y);
    glVertex2f(MinP.x, MaxP.y);

    glVertex2f(MinP.x, MaxP.y);
    glVertex2f(MinP.x, MinP.y);
}

inline void
OpenGLRectangle(v2 MinP, v2 MaxP, v4 PremulColor, v2 MinUV = V2(0, 0), v2 MaxUV = V2(1, 1))
{
    glBegin(GL_TRIANGLES);

    glColor4f(PremulColor.r, PremulColor.g, PremulColor.b, PremulColor.a);

    // NOTE: Lower triangle
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex2f(MinP.x, MinP.y);

    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex2f(MaxP.x, MinP.y);

    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex2f(MaxP.x, MaxP.y);

    // NOTE: Upper triangle
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex2f(MinP.x, MinP.y);

    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex2f(MaxP.x, MaxP.y);

    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex2f(MinP.x, MaxP.y);

    glEnd();
}

inline void
OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, int Pitch,
                    rectangle2i DrawRegion, v4 ClearColor, GLuint BlitTexture)
{
    Assert(Pitch == (Width*4));
    OpenGLBindFramebuffer(0, DrawRegion);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, BlitTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Width, Height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_TEXTURE_2D);

    glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    OpenGLSetScreenspace(Width, Height);

    // TODO: Decide how I want to handle aspect ratio - black bars or crop?

    v2 MinP = {0, 0};
    v2 MaxP = {(r32)Width, (r32)Height};
    v4 Color = {1, 1, 1, 1};

    OpenGLRectangle(MinP, MaxP, Color);

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);
}

internal void
OpenGLDrawBoundsRecursive(sort_sprite_bound *Bounds, u32 BoundIndex)
{
    sort_sprite_bound *Bound = Bounds + BoundIndex;
    if(!(Bound->Flags & Sprite_DebugBox))
    {
        v2 Center = GetCenter(Bound->ScreenArea);
        Bound->Flags |= Sprite_DebugBox;
        for(sprite_edge *Edge = Bound->FirstEdgeWithMeAsFront; Edge; Edge = Edge->NextEdgeWithSameFront)
        {
            sort_sprite_bound *Behind = Bounds + Edge->Behind;
            v2 BehindCenter = GetCenter(Behind->ScreenArea);
            glVertex2fv(Center.E);
            glVertex2fv(BehindCenter.E);

            OpenGLDrawBoundsRecursive(Bounds, Edge->Behind);
        }

        v2 SMin = GetMinCorner(Bound->ScreenArea);
        v2 SMax = GetMaxCorner(Bound->ScreenArea);
        OpenGLLineVertices(SMin, SMax);
    }
}

internal void *
OpenGLAllocateTexture(u32 Width, u32 Height, void *Data)
{
    GLuint Handle;
    glGenTextures(1, &Handle);
    glBindTexture(GL_TEXTURE_2D, Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat,
                 Width, Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBindTexture(GL_TEXTURE_2D, 0);

    Assert(sizeof(Handle) <= sizeof(void *));
    void *Result = PointerFromU32(void, Handle);
    return(Result);
}

internal void
OpenGLRenderCommands(game_render_commands *Commands, game_render_prep *Prep,
                     rectangle2i DrawRegion, u32 WindowWidth, u32 WindowHeight)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    b32 UseRenderTargets = (glBindFramebuffer != 0);

    u32 MaxRenderTargetIndex = UseRenderTargets ? Commands->MaxRenderTargetIndex : 0;
    if(MaxRenderTargetIndex >= GlobalFramebufferCount)
    {
        u32 NewFramebufferCount = Commands->MaxRenderTargetIndex + 1;
        Assert(NewFramebufferCount < ArrayCount(GlobalFramebufferHandles));
        u32 NewCount = NewFramebufferCount - GlobalFramebufferCount;
        glGenFramebuffers(NewCount, GlobalFramebufferHandles + GlobalFramebufferCount);
        for(u32 TargetIndex = GlobalFramebufferCount; TargetIndex <= NewFramebufferCount; ++TargetIndex)
        {
            GLuint TextureHandle = U32FromPointer(OpenGLAllocateTexture(GetWidth(DrawRegion), GetHeight(DrawRegion), 0));
            GlobalFramebufferTextures[TargetIndex] = TextureHandle;
            glBindFramebuffer(GL_FRAMEBUFFER, GlobalFramebufferHandles[TargetIndex]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureHandle, 0);
            GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            Assert(Status == GL_FRAMEBUFFER_COMPLETE);
        }

        GlobalFramebufferCount = NewFramebufferCount;
    }

    for(u32 TargetIndex = 0; TargetIndex <= MaxRenderTargetIndex; ++TargetIndex)
    {
        if(UseRenderTargets)
        {
            OpenGLBindFramebuffer(TargetIndex, DrawRegion);
        }

        if(TargetIndex == 0)
        {
            glScissor(0, 0, WindowWidth, WindowHeight);
        }
        else
        {
            glScissor(0, 0, GetWidth(DrawRegion), GetHeight(DrawRegion));
        }
        glClearColor(Commands->ClearColor.r, Commands->ClearColor.g, 
                     Commands->ClearColor.b, Commands->ClearColor.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    OpenGLSetScreenspace(Commands->Width, Commands->Height);

    u32 ClipRectIndex = 0xFFFFFFFF;
    u32 CurrentRenderTargetIndex = 0xFFFFFFFF;
    u32 *Entry = Prep->SortedIndices;
    for(u32 SortEntryIndex = 0; SortEntryIndex < Prep->SortedIndexCount; ++SortEntryIndex, ++Entry)
    {
        u32 HeaderOffset = *Entry;

        render_group_entry_header *Header = (render_group_entry_header *)
                                            (Commands->PushBufferBase + HeaderOffset);
        if(UseRenderTargets ||
           (Prep->ClipRects[Header->ClipRectIndex].RenderTargetIndex <= MaxRenderTargetIndex))
        {
            if(ClipRectIndex != Header->ClipRectIndex)
            {
                ClipRectIndex = Header->ClipRectIndex;
                Assert(ClipRectIndex < Commands->ClipRectCount);

                render_entry_cliprect *Clip = Prep->ClipRects + ClipRectIndex;

                rectangle2i ClipRect = Clip->Rect;
                if(CurrentRenderTargetIndex != Clip->RenderTargetIndex)
                {
                    CurrentRenderTargetIndex = Clip->RenderTargetIndex;
                    Assert(CurrentRenderTargetIndex <= MaxRenderTargetIndex);
                    if(UseRenderTargets)
                    {
                        OpenGLBindFramebuffer(CurrentRenderTargetIndex, DrawRegion);
                    }
                }

                if(!UseRenderTargets || (Clip->RenderTargetIndex == 0))
                {
                    ClipRect = Offset(ClipRect, DrawRegion.MinX, DrawRegion.MinY);
                }

                glScissor(ClipRect.MinX, ClipRect.MinY, 
                          ClipRect.MaxX - ClipRect.MinX, 
                          ClipRect.MaxY - ClipRect.MinY);
            }

            void *Data = (uint8_t *)Header + sizeof(*Header);
            switch(Header->Type)
            {
                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    if(Entry->Bitmap->Width && Entry->Bitmap->Height)
                    {
                        v2 XAxis = Entry->XAxis;
                        v2 YAxis = Entry->YAxis;
                        v2 MinP = Entry->P;

                        // TODO: Hold the frame if it is not ready with the texture?
#pragma warning(push)
#pragma warning(disable : 4312 4311 4302)
                        glBindTexture(GL_TEXTURE_2D, (GLuint)U32FromPointer(Entry->Bitmap->TextureHandle));
                        r32 OneTexelU = 1.0f / (r32)Entry->Bitmap->Width;
                        r32 OneTexelV = 1.0f / (r32)Entry->Bitmap->Height;
                        v2 MinUV = V2(OneTexelU, OneTexelV);
                        v2 MaxUV = V2(1.0f - OneTexelU, 1.0f - OneTexelV);

                        glBegin(GL_TRIANGLES);

                        // NOTE: This Value is not gamma corrected by OpenGL
                        
                        glColor4fv(Entry->PremulColor.E);

                        v2 MinXMinY = MinP;
                        v2 MinXMaxY = MinP + YAxis;
                        v2 MaxXMinY = MinP + XAxis;
                        v2 MaxXMaxY = MinP + XAxis + YAxis;

                        // NOTE: Lower triangle
                        glTexCoord2f(MinUV.x, MinUV.y);
                        glVertex2fv(MinXMinY.E);

                        glTexCoord2f(MaxUV.x, MinUV.y);
                        glVertex2fv(MaxXMinY.E);

                        glTexCoord2f(MaxUV.x, MaxUV.y);
                        glVertex2fv(MaxXMaxY.E);
                        
                        // NOTE: Upper triangle
                        glTexCoord2f(MinUV.x, MinUV.y);
                        glVertex2fv(MinXMinY.E);

                        glTexCoord2f(MaxUV.x, MaxUV.y);
                        glVertex2fv(MaxXMaxY.E);

                        glTexCoord2f(MinUV.x, MaxUV.y);
                        glVertex2fv(MinXMaxY.E);

                        glEnd();
                    }
                } break;

                case RenderGroupEntryType_render_entry_rectangle:
                {
                    render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
                    glDisable(GL_TEXTURE_2D);
                    OpenGLRectangle(Entry->P, Entry->P + Entry->Dim, Entry->PremulColor);
#if 1
                    // NOTE: Debug outlining
                    glBegin(GL_LINES);
                    glColor4f(0, 0, 0, Entry->PremulColor.a);
                    OpenGLLineVertices(Entry->P, Entry->P + Entry->Dim);
                    glEnd();
#endif
                    glEnable(GL_TEXTURE_2D);
                } break;

                case RenderGroupEntryType_render_entry_coordinate_system:
                {
                    render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
                } break;

                case RenderGroupEntryType_render_entry_blend_render_target:
                {
                    render_entry_blend_render_target *Entry = (render_entry_blend_render_target *)Data;
                    if(UseRenderTargets)
                    {
                        glBindTexture(GL_TEXTURE_2D, GlobalFramebufferTextures[Entry->SourceTargetIndex]);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        OpenGLRectangle(V2(0, 0), V2((r32)Commands->Width, (r32)Commands->Height),
                                        V4(1, 1, 1, Entry->Alpha));
                        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    }
                } break;

                InvalidDefaultCase;
            }
        }
    }

    if(UseRenderTargets)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if(GlobalShowSortGroups)
    {
        glDisable(GL_TEXTURE_2D);
        u32 BoundCount = Commands->SortEntryCount;
        sort_sprite_bound *Bounds = GetSortEntries(Commands);
        u32 GroupIndex = 0;
        for(u32 BoundIndex = 0; BoundIndex < BoundCount; ++BoundIndex)
        {
            sort_sprite_bound *Bound = Bounds + BoundIndex;
            if((Bounds->Offset != SPRITE_BARRIER_OFFSET_VALUE) &&
               !(Bound->Flags & Sprite_DebugBox))
            {
                v4 Color = V4(DebugColorTable[GroupIndex++ % ArrayCount(DebugColorTable)], 0.1f);
                if(Bound->Flags & Sprite_Cycle)
                {
                    Color.a = 1.0f;
                }
                Color.rgb *= Color.a;

                glBegin(GL_LINES);
                glColor4fv(Color.E);
                OpenGLDrawBoundsRecursive(Bounds, BoundIndex);
                glEnd();

                ++GroupIndex;
            }
        }
    }
#pragma warning(pop)
}

internal void
OpenGLManageTextures(texture_op *First)
{
    for(texture_op *Op = First; Op; Op = Op->Next)
    {
        if(Op->IsAllocate)
        {
            *Op->Allocate.ResultHandle = OpenGLAllocateTexture(Op->Allocate.Width, Op->Allocate.Height, 
                                                               Op->Allocate.Data);
        }
        else 
        {

            GLuint Handle = U32FromPointer(Op->Deallocate.Handle);
            glDeleteTextures(1, &Handle);
        }
    }
}
