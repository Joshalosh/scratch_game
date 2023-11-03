
inline void
OpenGLRectangle(v4 MinP, v4 MaxP, v4 Color)
{
    glBegin(GL_TRIANGLES);

    glColor4f(Color.r, Color.g, Color.b, Color.a);

    // NOTE: Lower triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.x, MinP.y);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(MaxP.x, MinP.y);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.x, MaxP.y);
    
    // NOTE: Upper triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.x, MinP.y);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.x, MaxP.y);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(MinP.x, MaxP.y);

    glEnd();
}

internal void
RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, rectangle2i ClipRect)
{
    IGNORED_TIMED_FUNCTION();

    glEnable(GL_TEXTURE_2D);

    glViewport(0, 0, OutputTarget->WindowWidth, OutputTarget->WindowHeight);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    r32 a = SafeRatio1(2.0f, (r32)OutputTarget->Width);
    r32 b = SafeRatio1(2.0f, (r32)OutputTarget->Height);
    r32 Proj[] =
    {
         a,  0,  0,  0,
         0,  b,  0,  0,
         0,  0,  1,  0,
        -1, -1,  0,  1,
    };
    glLoadMatrixf(Proj);

    u32 SortEntryCount = RenderGroup->PushBufferElementCount;
    tile_sort_entry *SortEntries = (tile_sort_entry *)(RenderGroup->PushBufferBase + RenderGroup->SortEntryAt);

    real32 NullPixelsToMetres = 1.0f;

    tile_sort_entry *Entry = SortEntries;
    for (u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
    {
        render_group_entry_header *Header = (render_group_entry_header *)
            (RenderGroup->PushBufferBase + Entry->PushBufferOffset);

        void *Data = (uint8_t *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupEntryType_render_entry_clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)Data;

                glClearColor(Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);
                glClear(GL_COLOR_BUFFER_BIT);
            } break;

            case RenderGroupEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                Assert(Entry->Bitmap);

                v2 XAxis = {1, 0};
                v2 YAxis = {0, 1};
                v2 MinP = Entry->P;
                v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;

                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
            } break;

            case RenderGroupEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
                OpenGLRectangle(Entry->P, Entry->P + Entry->Dim, Entry->Color);
            } break;

            case RenderGroupEntryType_render_entry_coordinate_system:
            {
                render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
            } break;

            InvalidDefaultCase;
        }
    }
}
