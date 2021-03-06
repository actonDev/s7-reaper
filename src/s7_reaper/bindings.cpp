#include "./bindings.hpp"
#include "reaper_plugin_functions.h"
#include "s7_reaper/gui.hpp"

namespace s7_reaper {

namespace bindings {
namespace sws {
void* FTOR_CF_ENUMERATE_ACTIONS = NULL; // set later upon binding

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE];

s7_pointer cf_enumarate_actions(s7_scheme* sc, s7_pointer args) {
    if(FTOR_CF_ENUMERATE_ACTIONS != NULL) {
//         int CF_EnumerateActions(int section, int index, char* nameOut, int nameOut_sz)
        // or int (*ftor)(int,int, char*, int) = ...;
        auto ftor  = (int (*)(int,int, char*, int)) FTOR_CF_ENUMERATE_ACTIONS;
        int section = s7_integer(s7_car(args));
        args = s7_cdr(args);
        int index = s7_integer(s7_car(args));
        int res = ftor(section, index, buffer, BUFFER_SIZE-1);

        // Lua: integer retval, string name = reaper.CF_EnumerateActions(integer section, integer index)
        return s7_list(sc,
                       2,
                       s7_make_integer(sc, res),
                       s7_make_string(sc, buffer));

    }
}

void bind(s7_scheme* sc, s7_pointer env) {
    s7_define(sc, env, s7_make_symbol(sc, "CF_EnumerateActions"),
              s7_make_function(sc, "CF_EnumerateActions", cf_enumarate_actions,
                               2, // req args
                               0, // optional args: thickness
                               false, // rest args
                               "(CF_EnumerateActions section idx) ret (retval:int name:string)"));
}

} // sws


namespace internal {

static s7_pointer c_object_equal(s7_scheme *sc, s7_pointer args)
{
    s7_pointer p1, p2;
    void *v1, *v2;
    p1 = s7_car(args);
    p2 = s7_cadr(args);
    if (p1 == p2)
        return(s7_t(sc));
    if (s7_c_object_type(p1) != s7_c_object_type(p2))
        return(s7_f(sc));
    v1 = s7_c_object_value(p1);
    v2 = s7_c_object_value(p2);
    return s7_make_boolean(sc, v1 == v2);
}

ReaProject* getCurrentProject() {
    // C: ReaProject* EnumProjects(int idx, char* projfnOutOptional, int projfnOutOptional_sz)
    //  idx=-1 for current project,projfn can be NULL if not interested in filename. use idx 0x40000000 for currently rendering project, if any.


    // return EnumProjects(-1, NULL, 0);
    // in the API when passing 0 it means the active project
    // so, that works as well
    return 0;
}

s7_pointer register_action(s7_scheme* sc, s7_pointer args) {

    const char* name = s7_string(s7_car(args));
    args = s7_cdr(args);
    s7_pointer fn = s7_car(args);
    // TODO maybe s7_gc_protect the fn?
    auto cb = [sc,fn]() {
        s7_call(sc, fn, s7_nil(sc));
    };

    // perhaps the environment of here is.. outlet of curlet?
    // at the moment keep in mind that I define this environment globablly as "rpr"
//     iplug::ReaperExtBase* rpr = (iplug::ReaperExtBase*)s7_c_pointer(s7_eval_c_string(sc, "(rpr 'ReaperExtBase*)"));
    iplug::ReaperExtBase* rpr = (iplug::ReaperExtBase*)s7_c_pointer(s7_eval_c_string_with_environment(sc, "ReaperExtBase*", s7_curlet(sc)));
    if(rpr == NULL) {
//         fprintf(stderr, "ReaperExtBase* is NULL, cannot register action\n");
        return s7_nil(sc);
    }

    printf("Registering action %s\n", name);
    rpr->RegisterAction(name, cb, true);
    return s7_nil(sc);
}

s7_pointer register_gui(s7_scheme* sc, s7_pointer args) {
    const char* name = s7_string(s7_car(args));
    args = s7_cdr(args);
    const char* script_file = s7_string(s7_car(args));

    auto pRec = (reaper_plugin_info_t*)s7_c_pointer(s7_eval_c_string_with_environment(sc, "*reaper_plugin_info_t*", s7_curlet(sc)));
    if(pRec == NULL) {
//         fprintf(stderr, "*reaper_plugin_info_t* is NULL, cannot register gui\n");
        return s7_nil(sc);
    }
    auto cb = [pRec, script_file]() {
        printf("GUI: running script file %s\n", script_file);
        s7_reaper::gui::init(pRec, script_file);
    };

    // perhaps the environment of here is.. outlet of curlet?
    // at the moment keep in mind that I define this environment globablly as "rpr"
//     iplug::ReaperExtBase* rpr = (iplug::ReaperExtBase*)s7_c_pointer(s7_eval_c_string(sc, "(rpr 'ReaperExtBase*)"));
    iplug::ReaperExtBase* rpr = (iplug::ReaperExtBase*)s7_c_pointer(s7_eval_c_string_with_environment(sc, "ReaperExtBase*", s7_curlet(sc)));
    if(rpr == NULL) {
//         fprintf(stderr, "ReaperExtBase* is NULL, cannot register action\n");
        return s7_nil(sc);
    }

    printf("registering gui %s:%s\n", name, script_file);
    rpr->RegisterAction(name, cb, true);
    return s7_nil(sc);
}

void bind(s7_scheme* sc, s7_pointer env) {
    s7_define(sc, env, s7_make_symbol(sc, "RegisterAction"),
              s7_make_function(sc, "RegisterAction", register_action,
                               2, 0, false, //
                               "(RegisterAction name fn) Adds a menu item inside reaper"));

    s7_define(sc, env, s7_make_symbol(sc, "RegisterGui"),
              s7_make_function(sc, "RegisterGui", register_gui,
                               2, 0, false, //
                               "(RegisterGui name script-file) CUSTOM (not part of reaper): Registers an action under name which runs a gui script"));
}

} // internal


namespace actions {
s7_pointer main_on_command(s7_scheme* sc, s7_pointer args) {
    // 2nd arg flags.. what is "flags" ??
    Main_OnCommand(s7_real(s7_car(args)), 0);
    return s7_nil(sc);
}

const char* help_NamedCommandLookup = "(NamedCommandLookup name-id)";
s7_pointer _NamedCommandLookup(s7_scheme* sc, s7_pointer args) {
    const char* name= s7_string(s7_car(args));
    int id = NamedCommandLookup(name);

    return s7_make_integer(sc, id);
}

const char* help_reverse_named_command_lookup = "(ReverseNamedCommandLookup id) Returns #f or the named id (string)";
s7_pointer reverse_named_command_lookup(s7_scheme* sc, s7_pointer args) {
    int id = s7_integer(s7_car(args));
    const char* name = ReverseNamedCommandLookup(id);
    if(name == NULL) {
        return s7_f(sc);
    }

    return s7_make_string(sc, name);
}

void bind(s7_scheme* sc, s7_pointer env) {
    // TODO should I keep the naming for the .."factory" functions?
    // and the create my own "wrappers" with normal dash-case?
    s7_define(sc, env, s7_make_symbol(sc, "Main_OnCommand"),
              s7_make_function(sc, "Main_OnCommand", main_on_command,
                               1, // req args
                               0, // optional args: thickness
                               false, // rest args
                               "(Main_OnCommand cmd-id)"));

    s7_define(sc, env, s7_make_symbol(sc, "ReverseNamedCommandLookup"),
              s7_make_function(sc, "ReverseNamedCommandLookup", reverse_named_command_lookup,
                               1, // req args
                               0, // optional args: thickness
                               false, // rest args
                               help_reverse_named_command_lookup));

    s7_define(sc, env, s7_make_symbol(sc, "NamedCommandLookup"),
              s7_make_function(sc, "NamedCommandLookup", _NamedCommandLookup,
                               1, 0, false,
                               help_NamedCommandLookup));

}
} // actions

namespace common {

s7_pointer update_arrage(s7_scheme* sc, s7_pointer) {
    UpdateArrange();
    return s7_nil(sc);
}

void bind(s7_scheme* sc, s7_pointer env) {
    s7_define(sc, env, s7_make_symbol(sc, "UpdateArrange"),
              s7_make_function(sc, "UpdateArrange", update_arrage,
                               0, 0, false,
                               "(UpdateArrange)"));

}
} // common

namespace tracks {

int tag_media_track(s7_scheme* sc) {
    s7_pointer res = s7_eval_c_string_with_environment(sc, "type-media-track", s7_curlet(sc));
    if(s7_is_integer(res)) {
        return s7_integer(res);
    }
    s7_error(sc,
             s7_make_symbol(sc, "error"),
             s7_cons(sc, s7_make_string(sc, "type-media-track not found"), s7_nil(sc)));
    return -1;
}


const char* help_CountTracks = "(CountTracks &optional ReaProject=0) TODO ReaProject always 0";
s7_pointer _CountTracks(s7_scheme* sc, s7_pointer) {
    return s7_make_integer(sc, CountTracks(0));
}

s7_pointer _GetMediaItem_Track(s7_scheme* sc, s7_pointer args) {
    MediaItem* item = (MediaItem*) s7_c_object_value(s7_car(args));
    if(item == NULL) {
        // or error?
        return s7_nil(sc);
    }
    MediaTrack* track = GetMediaItem_Track(item);

    return s7_make_c_object(sc, tag_media_track(sc), (void*)track);
}

s7_pointer _SetOnlyTrackSelected(s7_scheme* sc, s7_pointer args) {
    MediaTrack* track = (MediaTrack*) s7_c_object_value(s7_car(args));
    SetOnlyTrackSelected(track);

    return s7_nil(sc);
}

// SetTrackSelected
const char* help_SetTrackSelected = "(SetTrackSelected *media-track sel:bool)";
s7_pointer _SetTrackSelected(s7_scheme* sc, s7_pointer args) {
    MediaTrack* track = (MediaTrack*) s7_c_object_value(s7_car(args));
    args = s7_cdr(args);
    bool sel = s7_boolean(sc, s7_car(args));
    SetTrackSelected(track, sel);

    return s7_nil(sc);
}

const char* help_CountSelectedTracks = "(CountSelectedTracks &optional ReapProject*=0) TODO project is always 0";
s7_pointer _CountSelectedTracks(s7_scheme* sc, s7_pointer args) {
    return s7_make_integer(sc, CountSelectedTracks(0));
}

// GetSelectedTrack
const char* help_GetSelectedTrack = "(GetSelectedTrack ReapProject* idx) TODO project* always 0";
s7_pointer _GetSelectedTrack(s7_scheme* sc, s7_pointer args) {
    // ignoring first arg project
    args = s7_cdr(args);
    int idx = s7_integer(s7_car(args));
    MediaTrack* track = GetSelectedTrack(0,  idx);
    return s7_make_c_object(sc, tag_media_track(sc), (void*)track);
}

// returns <media-track name:"track name here">
s7_pointer to_string(s7_scheme* sc, s7_pointer args) {
    MediaTrack* track = (MediaTrack*) s7_c_object_value(s7_car(args));
    if(track == NULL) {
        return s7_make_string(sc, "null");
    }
    static char buffer[1024];
    GetSetMediaTrackInfo_String(track, "P_NAME", buffer, false);
    std::string str = "<media-track name:\"";
    str += buffer;
    str += "\">";
    return s7_make_string(sc, str.c_str());
}

void bind(s7_scheme* sc, s7_pointer env) {
    // types
    s7_int type = s7_make_c_type(sc, "<media-track>");
    s7_define(sc, env, s7_make_symbol(sc, "type-media-track"),
              s7_make_integer(sc, type));

    s7_c_type_set_to_string(sc, type, to_string);
    s7_c_type_set_is_equal(sc, type, internal::c_object_equal);

    s7_define(sc, env, s7_make_symbol(sc, "CountTracks"),
              s7_make_function(sc, "CountTracks", _CountTracks,
                               0, 1, false, // optional: ReaProject
                               help_CountTracks));
    s7_define(sc, env, s7_make_symbol(sc, "GetMediaItem_Track"),
              s7_make_function(sc, "GetMediaItem_Track", _GetMediaItem_Track,
                               1, 0, false,
                               "(GetMediaItem_Track *media-item)"));

    s7_define(sc, env, s7_make_symbol(sc, "SetOnlyTrackSelected"),
              s7_make_function(sc, "SetOnlyTrackSelected", _SetOnlyTrackSelected,
                               1, 0, false,
                               "(SetOnlyTrackSelected *media-track)"));

    s7_define(sc, env, s7_make_symbol(sc, "SetTrackSelected"),
              s7_make_function(sc, "SetTrackSelected", _SetTrackSelected,
                               2, 0, false,
                               help_SetTrackSelected));
    s7_define(sc, env, s7_make_symbol(sc, "CountSelectedTracks"),
              s7_make_function(sc, "CountSelectedTracks", _CountSelectedTracks,
                               0, 1, false, // optional: ReaProject
                               help_CountSelectedTracks));

    s7_define(sc, env, s7_make_symbol(sc, "GetSelectedTrack"),
              s7_make_function(sc, "GetSelectedTrack", _GetSelectedTrack,
                               2, 0, false,
                               help_GetSelectedTrack));
}

} // tracks

namespace takes {

int tag_media_take(s7_scheme* sc) {
    s7_pointer res = s7_eval_c_string(sc, "(rpr 'type-media-take)");
    if(s7_is_integer(res)) {
        return s7_integer(res);
    }
    s7_error(sc,
             s7_make_symbol(sc, "error"),
             s7_cons(sc, s7_make_string(sc, "type-media-take not defined in rpr"), s7_nil(sc)));
    return -1;
}

s7_pointer _GetSetMediaItemTakeInfo_String(s7_scheme* sc, s7_pointer args) {
    MediaItem_Take* take = (MediaItem_Take*) s7_c_object_value(s7_car(args));

    args = s7_cdr(args);
    const char* param_name = s7_string(s7_car(args));
    static char buffer[512];

    GetSetMediaItemTakeInfo_String(take, param_name, buffer, false);
    return s7_make_string(sc, buffer);
}

s7_pointer _GetActiveTake(s7_scheme* sc, s7_pointer args) {
    MediaItem* item = (MediaItem*) s7_c_object_value(s7_car(args));
    MediaItem_Take* take = GetActiveTake(item);

    return s7_make_c_object(sc, tag_media_take(sc), take);
}

void bind(s7_scheme* sc, s7_pointer env) {
    // types
    s7_int type = s7_make_c_type(sc, "<media-take>");
    s7_define(sc, env, s7_make_symbol(sc, "type-media-take"),
              s7_make_integer(sc, type));

//     s7_c_type_set_to_string(sc, type, to_string);
    s7_c_type_set_is_equal(sc, type, internal::c_object_equal);

    s7_define(sc, env, s7_make_symbol(sc, "GetSetMediaItemTakeInfo_String"),
              s7_make_function(sc, "GetSetMediaItemTakeInfo_String", _GetSetMediaItemTakeInfo_String,
                               2, 0, false,
                               "(GetSetMediaItemTakeInfo_String take* param-name)"));

    s7_define(sc, env, s7_make_symbol(sc, "GetActiveTake"),
              s7_make_function(sc, "GetActiveTake", _GetActiveTake,
                               1, 0, false,
                               "(GetActiveTake media-item*)"));
}

} // takes


namespace items {

int tag_media_item(s7_scheme* sc) {
    s7_pointer res = s7_eval_c_string(sc, "(rpr 'type-media-item)");
    if(s7_is_integer(res)) {
        return s7_integer(res);
    }
    s7_error(sc,
             s7_make_symbol(sc, "error"),
             s7_cons(sc, s7_make_string(sc, "type-media-item not defined in rpr"), s7_nil(sc)));
    return -1;
}

// returns <media-item {xyz-abc-xyz} active-take:"active take name">
s7_pointer to_string(s7_scheme* sc, s7_pointer args) {
    // checked? skipping for now
//     MediaItem* item = (MediaItem*) s7_c_object_value_checked(s7_car(args), tag_media_item(sc));
    MediaItem* item = (MediaItem*) s7_c_object_value(s7_car(args));
    if(item == NULL) {
        return s7_make_string(sc, "null");
    }
    MediaItem_Take* take = GetActiveTake(item);
    if(take == NULL) {
        return s7_make_string(sc, "null");
    }
    static char buffer[1024];
    GetSetMediaItemTakeInfo_String(take, "P_NAME", buffer, false);
    static char buffer_guid[32];
    GetSetMediaItemInfo_String(item, "GUID", buffer_guid, false);
    std::string str = "<media-item ";
    str += buffer_guid;
    str += " active-take:\"";
    str += buffer;
    str += "\">";
    return s7_make_string(sc, str.c_str());
}

const char* help_count_selected = "(CountSelectedMediaItems) TODO pass ReaProject";
s7_pointer count_selected(s7_scheme* sc, s7_pointer) {
    return s7_make_integer(sc, CountSelectedMediaItems(0));
}

const char* help_set_selected = "(SetMediaItemSelected *media-item selected?)";
s7_pointer set_selected(s7_scheme* sc, s7_pointer args) {
    MediaItem* item = (MediaItem*) s7_c_object_value(s7_car(args));
    args = s7_cdr(args);
    bool sel = s7_boolean(sc, s7_car(args));

    SetMediaItemSelected(item, sel);
    return s7_nil(sc);
}

const char* help_GetSelectedMediaItem = "(GetSelectedMediaItem ReaProject* idx) TODO ReaProject is always 0";
s7_pointer _GetSelectedMediaItem(s7_scheme* sc, s7_pointer args) {
    // skipping project for now
    args = s7_cdr(args);
    MediaItem* item = GetSelectedMediaItem(0, s7_integer(s7_car(args)));
    if(item == NULL) {
        printf("NULL item\n");
        return s7_nil(sc);
    }
    int tag = tag_media_item(sc);
    return s7_make_c_object(sc, tag, (void*) item);
}

const char* help_GetMediaItemInfo_Value = "(GetMediaItemInfo_Value item param-name)";
s7_pointer _GetMediaItemInfo_Value(s7_scheme* sc, s7_pointer args) {
    MediaItem* item = (MediaItem*) s7_c_object_value(s7_car(args));
    args = s7_cdr(args);
    const char* param = s7_string(s7_car(args));
    double value = GetMediaItemInfo_Value(item, param);

    return s7_make_real(sc, value);
}

void bind(s7_scheme* sc, s7_pointer env) {
    // types
    s7_int type = s7_make_c_type(sc, "<media-item>");
    s7_define(sc, env, s7_make_symbol(sc, "type-media-item"),
              s7_make_integer(sc, type));

    s7_c_type_set_to_string(sc, type, to_string);
    s7_c_type_set_is_equal(sc, type, internal::c_object_equal);

    // functions
    s7_define(sc, env, s7_make_symbol(sc, "CountSelectedMediaItems"),
              s7_make_function(sc, "CountSelectedMediaItems", count_selected,
                               0, 0, false,
                               help_count_selected));

    s7_define(sc, env, s7_make_symbol(sc, "GetSelectedMediaItem"),
              s7_make_function(sc, "GetSelectedMediaItem", _GetSelectedMediaItem,
                               2, 0, false, // proj, idx
                               help_GetSelectedMediaItem));

    s7_define(sc, env, s7_make_symbol(sc, "SetMediaItemSelected"),
              s7_make_function(sc, "SetMediaItemSelected", set_selected,
                               2, 0, false,
                               help_set_selected));

    // _GetMediaItemInfo_Value
    s7_define(sc, env, s7_make_symbol(sc, "GetMediaItemInfo_Value"),
              s7_make_function(sc, "GetMediaItemInfo_Value", _GetMediaItemInfo_Value,
                               2, 0, false,
                               help_GetMediaItemInfo_Value));

}
} // items

namespace time {

const char* help_GetSet_LoopTimeRange = "(GetSet_LoopTimeRange set? loop? start end allow-auto-seek)";
s7_pointer _GetSet_LoopTimeRange(s7_scheme* sc, s7_pointer args) {
    bool isset = s7_boolean(sc, s7_car(args));
    args = s7_cdr(args);
    bool isloop = s7_boolean(sc, s7_car(args));
    args = s7_cdr(args);
    double start = s7_real(s7_car(args));
    args = s7_cdr(args);
    double end = s7_real(s7_car(args));
    args = s7_cdr(args);
    bool allow_auto_seek = s7_boolean(sc, s7_car(args));

    GetSet_LoopTimeRange(isset, isloop, &start, &end, allow_auto_seek);

    if(!isset)
        return s7_nil(sc);
    // else, get: returns a list
    return s7_list(sc, 2,
                   s7_make_real(sc, start),
                   s7_make_real(sc, end));
}

const char* help_SetEditCurPos = "(SetEditCurPos time &optional move-view?=false seek-play?=false)";
s7_pointer _SetEditCurPos(s7_scheme* sc, s7_pointer args) {
    double time = s7_real(s7_car(args));
    bool move_view = false;
    bool seek_play = false;

    // 2 idios for handling these:
    // goto, or do{}while(false) hmm..
    {   // optional arguments
        args = s7_cdr(args);
        if(args == s7_nil(sc))
            goto ret;
        move_view = s7_boolean(sc, s7_car(args));

        args = s7_cdr(args);
        if(args == s7_nil(sc))
            goto ret;
        seek_play = s7_boolean(sc, s7_car(args));
    }
ret:
    SetEditCurPos(time, move_view, seek_play);
    return s7_nil(sc);

}

s7_pointer _GetCursorPosition(s7_scheme* sc, s7_pointer) {
    return s7_make_real(sc, GetCursorPosition());
}

void bind(s7_scheme* sc, s7_pointer env) {
    // functions
    s7_define(sc, env, s7_make_symbol(sc, "GetSet_LoopTimeRange"),
              s7_make_function(sc, "GetSet_LoopTimeRange", _GetSet_LoopTimeRange,
                               5, 0, false,
                               help_GetSet_LoopTimeRange));
    // _SetEditCurPos
    s7_define(sc, env, s7_make_symbol(sc, "SetEditCurPos"),
              s7_make_function(sc, "SetEditCurPos", _SetEditCurPos,
                               1, 2, false,
                               help_SetEditCurPos));
        s7_define(sc, env, s7_make_symbol(sc, "GetCursorPosition"),
              s7_make_function(sc, "GetCursorPosition", _GetCursorPosition,
                               0, 0, false,
                               "(GetCursorPosition)"));

}

} // range (time selection, loop)


void bind(iplug::ReaperExtBase* inst, reaper_plugin_info_t* pRec, s7_scheme* sc) {
    s7_pointer env = s7_inlet(sc, s7_nil(sc));
    s7_gc_protect(sc, env);

    printf("registering instance %p\n", inst);
    s7_define_constant_with_environment(sc, env, "ReaperExtBase*", s7_make_c_pointer(sc, (void*)inst));
    s7_define_constant_with_environment(sc, env, "*reaper_plugin_info_t*", s7_make_c_pointer(sc, (void*)pRec));

    int gErrorCount = 0;
    // as seen in ReaperExt_include_in_plug_src.h
#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)pRec->GetFunc(#x)))) gErrorCount++;

    IMPAPI(GetNumTracks);
    IMPAPI(CountTracks);
    IMPAPI(InsertTrackAtIndex);
    IMPAPI(Main_OnCommand);
    IMPAPI(UpdateArrange);
    IMPAPI(NamedCommandLookup);
    IMPAPI(ReverseNamedCommandLookup);


    // items
    IMPAPI(CountSelectedMediaItems);
    IMPAPI(GetSelectedMediaItem);
    IMPAPI(GetMediaItemInfo_Value)
//     IMPAPI(SetMediaItemInfo_Value);
//     IMPAPI(SetMediaItemLength);
//     IMPAPI(SetMediaItemPosition);
    IMPAPI(SetMediaItemSelected);
//     IMPAPI(SetMediaItemTake_Source);
//     IMPAPI(SetMediaItemTakeInfo_Value);
//     IMPAPI(GetSetMediaItemInfo);
    IMPAPI(GetSetMediaItemInfo_String);
//     IMPAPI(GetSetMediaItemTakeInfo);
//     IMPAPI(GetSetMediaItemTakeInfo_String);

    // takes
    IMPAPI(GetActiveTake);
    IMPAPI(GetSetMediaItemTakeInfo_String);

    // track
    IMPAPI(GetMediaItem_Track);
    IMPAPI(SetOnlyTrackSelected);
    IMPAPI(SetTrackSelected);
    IMPAPI(CountSelectedTracks);
    IMPAPI(GetSelectedTrack);
    IMPAPI(GetSetMediaTrackInfo_String);

    // time (time sel, loop sel, edit cursor)
    IMPAPI(GetSet_LoopTimeRange)
    IMPAPI(GetCursorPosition);
    IMPAPI(SetEditCurPos);

    //project
    IMPAPI(EnumProjects);

    // dynamic actions aka registered by other scripts (see SWS)
    sws::FTOR_CF_ENUMERATE_ACTIONS = pRec->GetFunc("CF_EnumerateActions");

    items::bind(sc, env);
    takes::bind(sc, env);
    tracks::bind(sc, env);
    common::bind(sc, env);
    sws::bind(sc, env);
    internal::bind(sc, env);
    actions::bind(sc, env);
    time::bind(sc, env); // time selection, loop selection, edit cursor

    s7_define_variable(sc, "rpr", env);
}
} // bindings
} // s7_reaper


