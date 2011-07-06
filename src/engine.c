/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "engine.h"
#include "zinnia.h"

typedef struct _IBusZinniaEngine IBusZinniaEngine;
typedef struct _IBusZinniaEngineClass IBusZinniaEngineClass;

struct _IBusZinniaEngine {
    IBusEngine parent;
    zinnia_character_t *character;
    zinnia_result_t *result;
    size_t stroke_count;
};

struct _IBusZinniaEngineClass {
    IBusEngineClass parent;
    zinnia_recognizer_t *recognizer;
};

/* functions prototype */
static void     ibus_zinnia_engine_class_init        (IBusZinniaEngineClass   *klass);
static void     ibus_zinnia_engine_init              (IBusZinniaEngine        *engine);
static void     ibus_zinnia_engine_destroy           (IBusZinniaEngine        *engine);
static void     ibus_zinnia_engine_candidate_clicked (IBusEngine              *engine,
                                                      guint                    index,
                                                      guint                    button,
                                                      guint                    state);
static void     ibus_zinnia_engine_process_hand_writing_event
                                                     (IBusEngine              *engine,
                                                      const gdouble           *coordinates,
                                                      guint                    coordinates_len);
static void     ibus_zinnia_engine_cancel_hand_writing
                                                     (IBusEngine              *engine,
                                                      guint                    n_strokes);
static void     ibus_zinnia_engine_reset             (IBusEngine              *engine);
static void     ibus_zinnia_engine_disable           (IBusEngine              *engine);
static void     ibus_zinnia_engine_focus_out         (IBusEngine              *engine);

G_DEFINE_TYPE (IBusZinniaEngine, ibus_zinnia_engine, IBUS_TYPE_ENGINE)

static const gint zinnia_xy = 1000;
static const gchar model_path[] = "/usr/share/tegaki/models/zinnia/handwriting-ja.model";
/* FIXME support Chinese and other languages */

static gint
normalize (gdouble x_or_y)
{
    gint result = (gint)(x_or_y * zinnia_xy);
    if (result < 0)
        return 0;
    if (result > zinnia_xy)
        return zinnia_xy;
    return result;
}

static void
maybe_init_zinnia (IBusZinniaEngine *zinnia)
{
    if (zinnia->stroke_count == 0) {
        g_return_if_fail (zinnia->character == NULL);
        g_return_if_fail (zinnia->result == NULL);

        zinnia->character = zinnia_character_new ();
        zinnia_character_clear (zinnia->character);
        zinnia_character_set_width (zinnia->character, zinnia_xy);
        zinnia_character_set_height (zinnia->character, zinnia_xy);
    }
}

static void
destroy_zinnia (IBusZinniaEngine *zinnia)
{
    if (zinnia->character) {
        zinnia_character_destroy (zinnia->character);
        zinnia->character = NULL;
    }
    if (zinnia->result != NULL) {
        zinnia_result_destroy (zinnia->result);
        zinnia->result = NULL;
    }
    zinnia->stroke_count = 0;
}

static void
ibus_zinnia_engine_class_init (IBusZinniaEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_zinnia_engine_destroy;

    engine_class->candidate_clicked = ibus_zinnia_engine_candidate_clicked;
    engine_class->process_hand_writing_event = ibus_zinnia_engine_process_hand_writing_event;
    engine_class->cancel_hand_writing = ibus_zinnia_engine_cancel_hand_writing;

    engine_class->reset = ibus_zinnia_engine_reset;
    engine_class->disable = ibus_zinnia_engine_disable;
    engine_class->focus_out = ibus_zinnia_engine_focus_out;

    klass->recognizer = zinnia_recognizer_new ();
    g_return_if_fail (zinnia_recognizer_open (klass->recognizer, model_path));
}

static void
ibus_zinnia_engine_init (IBusZinniaEngine *zinnia)
{
    if (g_object_is_floating (zinnia)) {
        g_object_ref_sink (zinnia);
    }
}

static void
ibus_zinnia_engine_destroy (IBusZinniaEngine *zinnia)
{
    destroy_zinnia (zinnia);
    ((IBusObjectClass *) ibus_zinnia_engine_parent_class)->destroy ((IBusObject *) zinnia);
}

static void
ibus_zinnia_engine_candidate_clicked (IBusEngine *engine,
                                      guint index,
                                      guint button,
                                      guint state)
{
    IBusZinniaEngine *zinnia = (IBusZinniaEngine *) engine;
    if (zinnia->result == NULL || index >= zinnia_result_size (zinnia->result)) {
        return;
    }
    IBusText *text = ibus_text_new_from_string (zinnia_result_value (zinnia->result, index));
    ibus_engine_commit_text (engine, text);
    ibus_engine_hide_lookup_table (engine);
    destroy_zinnia (zinnia);
}

static void
ibus_zinnia_engine_process_hand_writing_event (IBusEngine         *engine,
                                               const gdouble      *coordinates,
                                               guint               coordinates_len)
{
    static const gint max_candidates = 10;
    IBusZinniaEngine *zinnia = (IBusZinniaEngine *) engine;
    guint i;

    g_return_if_fail (coordinates_len >= 4);
    g_return_if_fail ((coordinates_len & 1) == 0);

    maybe_init_zinnia (zinnia);
    for (i = 1; i < coordinates_len; i += 2) {
        zinnia_character_add (zinnia->character,
                              zinnia->stroke_count,
                              normalize(coordinates[i - 1]),
                              normalize(coordinates[i]));
    }
    zinnia->stroke_count++;

    if (zinnia->result != NULL) {
        zinnia_result_destroy (zinnia->result);
    }

    IBusZinniaEngineClass *klass = G_TYPE_INSTANCE_GET_CLASS (zinnia,
                                                              IBusZinniaEngine,
                                                              IBusZinniaEngineClass);
    zinnia->result = zinnia_recognizer_classify (klass->recognizer,
                                                 zinnia->character,
                                                 max_candidates);

    if (zinnia->result == NULL || zinnia_result_size (zinnia->result) == 0) {
        ibus_engine_hide_lookup_table (engine);
    } else {
        IBusLookupTable *table = ibus_lookup_table_new (max_candidates, /* page size */
                                                        0, /* cursur pos */
                                                        FALSE, /* cursor visible */
                                                        TRUE); /* round */
        ibus_lookup_table_set_orientation (table, IBUS_ORIENTATION_VERTICAL);

        for (i = 0; i < zinnia_result_size (zinnia->result); i++) {
            IBusText *text = ibus_text_new_from_string (zinnia_result_value (zinnia->result, i));
            ibus_lookup_table_append_candidate (table, text);
        }
        ibus_engine_update_lookup_table (engine, table, TRUE);
    }
}

static void
ibus_zinnia_engine_cancel_hand_writing (IBusEngine              *engine,
                                        guint                    n_strokes)
{
    IBusZinniaEngine *zinnia = (IBusZinniaEngine *) engine;
    ibus_engine_hide_lookup_table (engine);
    destroy_zinnia (zinnia);

    /* FIXME support n_strokes != 0 cases */
}

static void
ibus_zinnia_engine_reset (IBusEngine *engine)
{
    ibus_zinnia_engine_cancel_hand_writing (engine, 0);
}

static void
ibus_zinnia_engine_disable (IBusEngine *engine)
{
    ibus_zinnia_engine_cancel_hand_writing (engine, 0);
}

static void
ibus_zinnia_engine_focus_out (IBusEngine *engine)
{
    ibus_zinnia_engine_cancel_hand_writing (engine, 0);
}
