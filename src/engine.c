/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "engine.h"
#include "zinnia.h"

typedef struct _IBusZinniaEngine IBusZinniaEngine;
typedef struct _IBusZinniaEngineClass IBusZinniaEngineClass;

struct _IBusZinniaEngine {
    IBusEngine parent;
    zinnia_recognizer_t *recognizer;
    zinnia_character_t *character;
    zinnia_result_t *result;
    size_t stroke_count;
};

struct _IBusZinniaEngineClass {
    IBusEngineClass parent;
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

G_DEFINE_TYPE (IBusZinniaEngine, ibus_zinnia_engine, IBUS_TYPE_ENGINE)

static const gint zinnia_xy = 1000;
static const gchar model_path[] = "/usr/share/tegaki/models/zinnia/handwriting-ja.model";
/* FIXME support Chinese and other languages */

static gint
normalize(gdouble x_or_y)
{
    gint result = (gint)(x_or_y * zinnia_xy);
    if (result < 0)
        return 0;
    if (result > zinnia_xy)
        return zinnia_xy;
    return result;
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
}

static void
ibus_zinnia_engine_init (IBusZinniaEngine *zinnia)
{
    zinnia->recognizer = zinnia_recognizer_new ();
    zinnia->character = zinnia_character_new ();

    g_return_if_fail (zinnia_recognizer_open (zinnia->recognizer, model_path));
    zinnia_character_clear (zinnia->character);
    zinnia_character_set_width (zinnia->character, zinnia_xy);
    zinnia_character_set_height (zinnia->character, zinnia_xy);
    zinnia->result = NULL;
    zinnia->stroke_count = 0;
}

static void
ibus_zinnia_engine_destroy (IBusZinniaEngine *zinnia)
{
    zinnia_character_destroy (zinnia->character);
    zinnia_recognizer_destroy (zinnia->recognizer);
    if (zinnia->result != NULL) {
        zinnia_result_destroy (zinnia->result);
    }
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
    zinnia->result = zinnia_recognizer_classify (zinnia->recognizer,
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

    zinnia_character_clear (zinnia->character);
    zinnia->stroke_count = 0;
    ibus_engine_hide_lookup_table (engine);

    /* FIXME support n_strokes != 0 cases */
}
