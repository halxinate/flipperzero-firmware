#include "scenes/updater_scene.h"
//#include "scenes/updater_scene_i.h"
#include "updater_i.h"

#include <storage/storage.h>
//#include <gui/view_stack.h>
#include <gui/view_dispatcher.h>
#include <furi.h>
#include <furi_hal.h>
#include <portmacro.h>
#include <stdint.h>

static bool updater_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Updater* updater = (Updater*)context;
    return scene_manager_handle_custom_event(updater->scene_manager, event);
}

static void updater_tick_event_callback(void* context) {
    furi_assert(context);
    Updater* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

static bool updater_back_event_callback(void* context) {
    furi_assert(context);
    Updater* updater = (Updater*)context;
    return scene_manager_handle_back_event(updater->scene_manager);
}

static void status_update_cb(const char* message, const uint8_t progress, void* context) {
    UpdaterMainView* main_view = context;
    updater_main_model_set_state(main_view, message, progress);
}

Updater* updater_alloc() {
    Updater* updater = malloc(sizeof(Updater));

    updater->storage = furi_record_open("storage");

    updater->gui = furi_record_open("gui");
    //updater->scene_thread = furi_thread_alloc();
    updater->view_dispatcher = view_dispatcher_alloc();
    updater->scene_manager = scene_manager_alloc(&updater_scene_handlers, updater);

    view_dispatcher_enable_queue(updater->view_dispatcher);
    view_dispatcher_attach_to_gui(
        updater->view_dispatcher, updater->gui, ViewDispatcherTypeWindow);

    view_dispatcher_set_event_callback_context(updater->view_dispatcher, updater);
    view_dispatcher_set_custom_event_callback(
        updater->view_dispatcher, updater_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        updater->view_dispatcher, updater_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        updater->view_dispatcher, updater_tick_event_callback, 500);

    updater->main_view = updater_main_alloc();

    updater->update_task = update_task_alloc();
    update_task_set_progress_cb(updater->update_task, status_update_cb, updater->main_view);

    view_dispatcher_add_view(
        updater->view_dispatcher, UpdaterViewMain, updater_main_get_view(updater->main_view));

    return updater;
}

void updater_free(Updater* updater) {
    furi_assert(updater);

    update_task_set_progress_cb(updater->update_task, NULL, NULL);
    update_task_free(updater->update_task);

    view_dispatcher_remove_view(updater->view_dispatcher, UpdaterViewMain);

    view_dispatcher_free(updater->view_dispatcher);
    scene_manager_free(updater->scene_manager);

    //view_stack_free(updater->main_view_stack);
    updater_main_free(updater->main_view);

    furi_record_close("gui");
    updater->gui = NULL;

    furi_record_close("storage");
    updater->storage = NULL;

    //furi_thread_free(updater->scene_thread);

    furi_record_close("menu");

    free(updater);
}

int32_t updater_srv(void* p) {
    Updater* updater = updater_alloc();

    //if(!furi_hal_rtc_is_flag_set(FuriHalRtcFlagExecuteUpdate)) {
    //    //furi_hal_usb_disable();
    //    // TODO: restart
    //    //scene_manager_set_scene_state(
    //    //    updater->scene_manager, UpdaterSceneMain, UpdaterMainSceneStateLockedWithPin);
    //}

    scene_manager_next_scene(updater->scene_manager, UpdaterSceneMain);

    view_dispatcher_run(updater->view_dispatcher);
    updater_free(updater);

    return 0;
}