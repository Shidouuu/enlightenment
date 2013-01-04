#include <e.h>
#include <EDBus.h>
#include "e_mod_main.h"
#include "ebluez4.h"

#define ILIST_HEADER "Devices Found"

/* Local Variables */
static Eina_List *instances = NULL;
static E_Module *mod = NULL;
static char tmpbuf[1024];

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Bluez4"};

/* Local Functions */
static void
_ebluez4_cb_pair(void *data, void *data2 __UNUSED__)
{
   Instance *inst = data;
   const char *addr = e_widget_ilist_selected_value_get(inst->found_list);

   if(!addr)
     return;
   e_gadcon_popup_hide(inst->popup);
   ebluez4_connect_to_device(addr);
}

static Eina_Bool
_ebluez4_cb_stop_search(void *data)
{
   Instance *inst = data;
   ebluez4_stop_discovery();
   e_widget_disabled_set(inst->bt, 0);
   DBG("Stopping discovery...");
   return ECORE_CALLBACK_CANCEL;
}

static void
_ebluez4_cb_search(void *data, void *data2 __UNUSED__)
{
   Instance *inst = data;
   e_widget_ilist_clear(inst->found_list);
   e_widget_ilist_header_append(inst->found_list, NULL, "Devices Found");
   ebluez4_start_discovery();
   e_widget_disabled_set(inst->bt, 1);
   ecore_timer_add(60, _ebluez4_cb_stop_search, inst);
   DBG("Starting discovery...");
}

static void
_ebluez4_popup_new(Instance *inst)
{
   Evas_Object *list, *tb, *bt2;
   Evas_Coord mw, mh;
   Evas *evas;

   EINA_SAFETY_ON_FALSE_RETURN(inst->popup == NULL);

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   list = e_widget_list_add(evas, 0, 0);
   inst->found_list = e_widget_ilist_add(evas, 0, 0, NULL);
   e_widget_list_object_append(list, inst->found_list, 1, 1, 0.5);

   e_widget_ilist_header_append(inst->found_list, NULL, ILIST_HEADER);

   inst->bt = e_widget_button_add(evas, "Search Devices", NULL,
                                  _ebluez4_cb_search, inst, NULL);
   bt2 = e_widget_button_add(evas, "Connect", NULL, _ebluez4_cb_pair, inst, NULL);

   tb = e_widget_table_add(evas, 0);

   e_widget_table_object_append(tb, inst->bt, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(tb, bt2, 1, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(list, tb, 1, 0, 0.5);

   e_widget_size_min_get(list, &mw, &mh);
   if (mh < 220)
     mh = 220;
   if (mw < 250)
     mw = 250;
   e_widget_size_min_set(list, mw, mh);

   e_gadcon_popup_content_set(inst->popup, list);
   e_gadcon_popup_show(inst->popup);
}

static void
_ebluez4_popup_del(Instance *inst)
{
   if (!inst->popup) return;
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_ebluez4_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst = NULL;
   Evas_Event_Mouse_Down *ev = event;

   if (!(inst = data)) return;
   if (ev->button != 1) return;
   if (!ctxt->adap_obj) return;

   if (!inst->popup)
     _ebluez4_popup_new(inst);
   else if (inst->popup->win->visible)
     e_gadcon_popup_hide(inst->popup);
   else
     e_gadcon_popup_show(inst->popup);
}

static void
_ebluez4_set_mod_icon(Evas_Object *base)
{
   char edj_path[4096];
   char *group;

   snprintf(edj_path, sizeof(edj_path), "%s/e-module-bluez4.edj", mod->dir);
   if (ctxt->adap_obj)
     group = "modules/bluez4/main";
   else
     group = "modules/bluez4/inactive";

   if (!e_theme_edje_object_set(base, "base/theme/modules/bluez4", group))
     edje_object_file_set(base, edj_path, group);
}

/* Gadcon */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst = NULL;

   inst = E_NEW(Instance, 1);

   inst->o_bluez4 = edje_object_add(gc->evas);
   _ebluez4_set_mod_icon(inst->o_bluez4);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_bluez4);
   inst->gcc->data = inst;

   evas_object_event_callback_add(inst->o_bluez4, EVAS_CALLBACK_MOUSE_DOWN,
                                  _ebluez4_cb_mouse_down, inst);

   instances = eina_list_append(instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = NULL;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);

   if (inst->o_bluez4)
     {
        evas_object_event_callback_del(inst->o_bluez4, EVAS_CALLBACK_MOUSE_DOWN,
                                       _ebluez4_cb_mouse_down);
        evas_object_del(inst->o_bluez4);
     }

   _ebluez4_popup_del(inst);

   E_FREE(inst);
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
    snprintf(tmpbuf, sizeof(tmpbuf), "bluez4.%d", eina_list_count(instances));
    return tmpbuf;
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class)
{
   return "Bluez4";
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o = NULL;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/e-module-bluez4.edj", mod->dir);

   o = edje_object_add(evas);

   edje_object_file_set(o, buf, "icon");

   return o;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, "bluez4",
     {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon,
          _gc_id_new, NULL, NULL},
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* Module Functions */
EAPI void *
e_modapi_init(E_Module *m)
{
   mod = m;

   ebluez4_edbus_init();

   e_gadcon_provider_register(&_gc_class);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   ebluez4_edbus_shutdown();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/* Public Functions */
void
ebluez4_disabled_set_all_search_buttons(Eina_Bool disabled)
{
   Eina_List *iter;
   Instance *inst;

   EINA_LIST_FOREACH(instances, iter, inst)
     e_widget_disabled_set(inst->bt, disabled);
}

void
ebluez4_append_to_instances(const char *addr, const char *name)
{
   Eina_List *iter;
   Instance *inst;

   EINA_LIST_FOREACH(instances, iter, inst)
     e_widget_ilist_append(inst->found_list, NULL, name, NULL, NULL, addr);
}

void
ebluez4_update_inst(Evas_Object *dest, Eina_List *src)
{
   Device *dev;
   Eina_List *iter;

   e_widget_ilist_freeze(dest);
   e_widget_ilist_clear(dest);

   e_widget_ilist_header_append(dest, NULL, ILIST_HEADER);
   EINA_LIST_FOREACH(src, iter, dev)
     if (!dev->connected)
       e_widget_ilist_append(dest, NULL, dev->name, NULL, NULL, dev->addr);

   e_widget_ilist_thaw(dest);
   e_widget_ilist_go(dest);
}

void
ebluez4_update_instances(Eina_List *src)
{
   Eina_List *iter;
   Instance *inst;

   EINA_LIST_FOREACH(instances, iter, inst)
     if (inst->found_list)
       ebluez4_update_inst(inst->found_list, src);
}

void
ebluez4_update_all_gadgets_visibility()
{
   Eina_List *iter;
   Instance *inst;

   if (ctxt->adap_obj)
     EINA_LIST_FOREACH(instances, iter, inst)
       _ebluez4_set_mod_icon(inst->o_bluez4);
   else
     EINA_LIST_FOREACH(instances, iter, inst)
       {
          _ebluez4_set_mod_icon(inst->o_bluez4);
          e_gadcon_popup_hide(inst->popup);
       }
}
