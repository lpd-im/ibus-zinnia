#include <string.h>
#include <glib.h>
#include "zinnia_component.h"

static IBusEngineDesc *
ibus_zinnia_engine_new (void)
{
    IBusEngineDesc *engine = NULL;
    engine = ibus_engine_desc_new_varargs ("name", "zinnia-japanese",
                                           "longname", "Japanese hand-writing engine",
                                           "description", "Japanese hand-writing engine",
                                           "language", "ja",
                                           "license", "Apache",
                                           "author", "The Chromium OS authors",
                                           "hotkeys", "",
                                           "rank", 0,
                                           "layout", "us",
                                           NULL);
    return engine;
}

GList *
ibus_zinnia_list_engines (void)
{
    GList *engines = NULL;
    engines = g_list_append (engines, ibus_zinnia_engine_new ());
    return engines;
}

IBusComponent *
ibus_zinnia_get_component (void)
{
    GList *engines, *p;
    IBusComponent *component;

    component = ibus_component_new ("com.google.IBus.Zinnia",
                                    "Zinnia hand-writing Component",
                                    "0.0.0",
                                    "Apache",
                                    "The Chromium OS Authors",
                                    "http://github.com/yusukes/ibus-zinnia",
                                    "",  // exec
                                    "ibus-zinnia");

    engines = ibus_zinnia_list_engines ();

    for (p = engines; p != NULL; p = p->next) {
        ibus_component_add_engine (component, IBUS_ENGINE_DESC(p->data));
    }

    g_list_free (engines);
    return component;
}
