member_definition MembersOf_sim_entity[] = 
{
    {MetaType_world_chunk, "OldChunk", (u64)&((sim_entity *)0)->OldChunk},
    {MetaType_uint32_t, "StorageIndex", (u64)&((sim_entity *)0)->StorageIndex},
    {MetaType_bool32, "Updatable", (u64)&((sim_entity *)0)->Updatable},
    {MetaType_entity_type, "Type", (u64)&((sim_entity *)0)->Type},
    {MetaType_uint32_t, "Flags", (u64)&((sim_entity *)0)->Flags},
    {MetaType_v3, "P", (u64)&((sim_entity *)0)->P},
    {MetaType_v3, "dP", (u64)&((sim_entity *)0)->dP},
    {MetaType_real32, "DistanceLimit", (u64)&((sim_entity *)0)->DistanceLimit},
    {MetaType_sim_entity_collision_volume_group, "Collision", (u64)&((sim_entity *)0)->Collision},
    {MetaType_real32, "FacingDirection", (u64)&((sim_entity *)0)->FacingDirection},
    {MetaType_real32, "tBob", (u64)&((sim_entity *)0)->tBob},
    {MetaType_int32_t, "dAbsTileZ", (u64)&((sim_entity *)0)->dAbsTileZ},
    {MetaType_uint32_t, "HitPointMax", (u64)&((sim_entity *)0)->HitPointMax},
    {MetaType_hit_point, "HitPoint", (u64)&((sim_entity *)0)->HitPoint},
    {MetaType_entity_reference, "Sword", (u64)&((sim_entity *)0)->Sword},
    {MetaType_v2, "WalkableDim", (u64)&((sim_entity *)0)->WalkableDim},
    {MetaType_real32, "WalkableHeight", (u64)&((sim_entity *)0)->WalkableHeight},
};
member_definition MembersOf_sim_region[] = 
{
    {MetaType_world, "World", (u64)&((sim_region *)0)->World},
    {MetaType_real32, "MaxEntityRadius", (u64)&((sim_region *)0)->MaxEntityRadius},
    {MetaType_real32, "MaxEntityVelocity", (u64)&((sim_region *)0)->MaxEntityVelocity},
    {MetaType_world_position, "Origin", (u64)&((sim_region *)0)->Origin},
    {MetaType_rectangle3, "Bounds", (u64)&((sim_region *)0)->Bounds},
    {MetaType_rectangle3, "UpdatableBounds", (u64)&((sim_region *)0)->UpdatableBounds},
    {MetaType_uint32_t, "MaxEntityCount", (u64)&((sim_region *)0)->MaxEntityCount},
    {MetaType_uint32_t, "EntityCount", (u64)&((sim_region *)0)->EntityCount},
    {MetaType_sim_entity, "Entities", (u64)&((sim_region *)0)->Entities},
    {MetaType_sim_entity_hash, "Hash", (u64)&((sim_region *)0)->Hash},
};
member_definition MembersOf_rectangle2[] = 
{
    {MetaType_v2, "Min", (u64)&((rectangle2 *)0)->Min},
    {MetaType_v2, "Max", (u64)&((rectangle2 *)0)->Max},
};
member_definition MembersOf_rectangle3[] = 
{
    {MetaType_v3, "Min", (u64)&((rectangle3 *)0)->Min},
    {MetaType_v3, "Max", (u64)&((rectangle3 *)0)->Max},
};
#define META_HANDLE_TYPE_DUMP(MemberPtr) \
    case MetaType_rectangle3: {DEBUGDumpStruct(ArrayCount(MembersOf_rectangle3), MembersOf_rectangle3, MemberPtr);} break; \
    case MetaType_rectangle2: {DEBUGDumpStruct(ArrayCount(MembersOf_rectangle2), MembersOf_rectangle2, MemberPtr);} break; \
    case MetaType_sim_region: {DEBUGDumpStruct(ArrayCount(MembersOf_sim_region), MembersOf_sim_region, MemberPtr);} break; \
    case MetaType_sim_entity: {DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity), MembersOf_sim_entity, MemberPtr);} break; 
