/* -*- mode: C; c-file-style: "gnu" -*- */

#include <config.h>
#include <string.h>
#include <dbus/dbus.h>
#include "gconf-dbus-utils.h"

GConfValue* gconf_value_decode (const gchar* encoded);
gchar * gconf_value_encode (GConfValue* val);

/*

Notify (ConfigDatabase db,
        Array<string>  schema_names,
        Array<bool>    is_defaults,
        Array<bool>    is_writables,
	v1, v2, v3, ...);

Lookup (Array<string>  keys,
        string         locale
        boolean        use_schema_default,
	// out:
        Array<string>  schema_names,
        Array<bool>    is_defaults,
        Array<bool>    is_writables,
	v1, v2, v3, ...);

Set    (Array<string> keys,
        v1, v2, v3, ...);

AllEntries (string         dir,
            string         locale,
	    // out:
            Array<string>  schema_names,
            Array<bool>    is_defaults,
            Array<bool>    is_writables,
	    v1, v2, v3, ...);

AllDirs    (string         dir,
            string         locale,
	    // out:
            Array<string>  schema_names,
            Array<bool>    is_defaults,
            Array<bool>    is_writables,
	    v1, v2, v3, ...);

*/

GConfValue *
gconf_value_from_dict (DBusMessageIter *iter,
		       const char      *key)
{
  DBusMessageIter dict;
  gchar *str;
  gboolean found = FALSE;
  GConfValue *value;

  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (key != NULL, NULL);
  
  g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_DICT);
     
  dbus_message_iter_init_dict_iterator (iter, &dict);

  while (1)
    {
      str = dbus_message_iter_get_dict_key (&dict);

      if (str != NULL && strcmp (str, key) == 0)
	{
	  dbus_free (str);
	  found = TRUE;
	  break;
	}

      dbus_free (str);

      if (!dbus_message_iter_next (&dict))
	break;
    } 

  if (!found)
    {
      g_error ("Unknown dict key.");
      return NULL;
    }
  
  switch (dbus_message_iter_get_arg_type (&dict))
    {
    case DBUS_TYPE_STRING:
      {
	const char *str;
	value = gconf_value_new (GCONF_VALUE_STRING);

	str = dbus_message_iter_get_string (&dict);
	gconf_value_set_string (value, str);

	return value;
      }
    case DBUS_TYPE_INT32:
      {
	dbus_int32_t val;
	
	value = gconf_value_new (GCONF_VALUE_INT);

	val = dbus_message_iter_get_int32 (&dict);
	gconf_value_set_int (value, val);

	return value;
      }
    case DBUS_TYPE_DOUBLE:
      {
	double val;
	
	value = gconf_value_new (GCONF_VALUE_FLOAT);

	val = dbus_message_iter_get_double (&dict);
	gconf_value_set_float (value, val);

	return value;
      }
    case DBUS_TYPE_BOOLEAN:
      {
	dbus_bool_t val;
	
	value = gconf_value_new (GCONF_VALUE_BOOL);

	val = dbus_message_iter_get_boolean (&dict);
	gconf_value_set_bool (value, val);

	return value;
      }
    default:
      return NULL;
    }
}

void
set_dict_value_from_gconf_value (DBusMessageIter *dict,
				 const char      *key,
				 GConfValue      *value)
{
  dbus_message_iter_append_dict_key (dict, key);

  switch (value->type)
    {
    case GCONF_VALUE_STRING:
      dbus_message_iter_append_string (dict, gconf_value_get_string (value));
      break;
    case GCONF_VALUE_INT:
      dbus_message_iter_append_int32 (dict, gconf_value_get_int (value));
      break;
    case GCONF_VALUE_FLOAT:
      dbus_message_iter_append_double (dict, gconf_value_get_float (value));
      break;
    case GCONF_VALUE_BOOL:
      dbus_message_iter_append_boolean (dict, gconf_value_get_bool (value));
      break;
    default:
      g_assert_not_reached ();
    }
}

void
gconf_dbus_message_append_gconf_schema (DBusMessage       *message,
					const GConfSchema *schema)
{
  DBusMessageIter iter, dict;
  GConfValue *default_val;

 dbus_message_append_iter_init (message, &iter);
  
  if (!schema)
    {
      dbus_message_iter_append_nil (&iter);
      return;
    }
  
  dbus_message_iter_append_dict (&iter, &dict);

  dbus_message_iter_append_dict_key (&dict, "type");
  dbus_message_iter_append_int32 (&dict, gconf_schema_get_type (schema));

  dbus_message_iter_append_dict_key (&dict, "list_type");
  dbus_message_iter_append_int32 (&dict, gconf_schema_get_list_type (schema));

  dbus_message_iter_append_dict_key (&dict, "car_type");
  dbus_message_iter_append_int32 (&dict, gconf_schema_get_car_type (schema));

  dbus_message_iter_append_dict_key (&dict, "cdr_type");
  dbus_message_iter_append_int32 (&dict, gconf_schema_get_cdr_type (schema));

  dbus_message_iter_append_dict_key (&dict, "locale");
  dbus_message_iter_append_string (&dict, gconf_schema_get_locale (schema) ?
				   gconf_schema_get_locale (schema) : "");
  
  dbus_message_iter_append_dict_key (&dict, "short_desc");
  dbus_message_iter_append_string (&dict, gconf_schema_get_short_desc (schema) ?
				   gconf_schema_get_short_desc (schema) : "");

  dbus_message_iter_append_dict_key (&dict, "long_desc");
  dbus_message_iter_append_string (&dict, gconf_schema_get_long_desc (schema) ?
				   gconf_schema_get_long_desc (schema) : "");

  dbus_message_iter_append_dict_key (&dict, "owner");
  dbus_message_iter_append_string (&dict, gconf_schema_get_owner (schema) ?
				   gconf_schema_get_owner (schema) : "");
  
  default_val = gconf_schema_get_default_value (schema);
  
  /* We don't need to do this, but it's much simpler */
  if (default_val)
    {
      gchar *encoded = gconf_value_encode (default_val);
      g_assert (encoded != NULL);

      dbus_message_iter_append_dict_key (&dict, "default_value");
      dbus_message_iter_append_string (&dict, encoded);

      g_free (encoded);
    }
}

GConfValue *
gconf_dbus_create_gconf_value_from_dict (DBusMessageIter *dict)
{
  DBusMessageIter child_iter;
  gchar *name;
  GConfValue *value = NULL;

  g_assert (dbus_message_iter_get_arg_type (dict) == DBUS_TYPE_DICT);
  
  dbus_message_iter_init_dict_iterator (dict, &child_iter);
  name = dbus_message_iter_get_dict_key (&child_iter);

  /* Check if we've got a schema dict */
  if (strcmp (name, "type") == 0)
    {
      dbus_int32_t type, list_type, car_type, cdr_type;
      gchar *tmp;
      GConfSchema *schema;

      type = dbus_message_iter_get_int32 (&child_iter);
      dbus_message_iter_next (&child_iter);

      list_type = dbus_message_iter_get_int32 (&child_iter);
      dbus_message_iter_next (&child_iter);

      car_type = dbus_message_iter_get_int32 (&child_iter);
      dbus_message_iter_next (&child_iter);

      cdr_type = dbus_message_iter_get_int32 (&child_iter);
      dbus_message_iter_next (&child_iter);

      schema = gconf_schema_new ();

      gconf_schema_set_type (schema, type);
      gconf_schema_set_list_type (schema, list_type);
      gconf_schema_set_car_type (schema, car_type);
      gconf_schema_set_cdr_type (schema, cdr_type);
      
      value = gconf_value_new (GCONF_VALUE_SCHEMA);
      gconf_value_set_schema_nocopy (value, schema);
      
      tmp = dbus_message_iter_get_string (&child_iter);
      dbus_message_iter_next (&child_iter);
      if (*tmp != '\0')
	gconf_schema_set_locale (schema, tmp);
      dbus_free (tmp);

      tmp = dbus_message_iter_get_string (&child_iter);
      dbus_message_iter_next (&child_iter);
      if (*tmp != '\0')
	gconf_schema_set_short_desc (schema, tmp);
      dbus_free (tmp);

      tmp = dbus_message_iter_get_string (&child_iter);
      dbus_message_iter_next (&child_iter);
      if (*tmp != '\0')
	gconf_schema_set_long_desc (schema, tmp);
      dbus_free (tmp);

      tmp = dbus_message_iter_get_string (&child_iter);
      dbus_message_iter_next (&child_iter);
      if (*tmp != '\0')
	gconf_schema_set_owner (schema, tmp);
      dbus_free (tmp);

      tmp = dbus_message_iter_get_string (&child_iter);
      dbus_message_iter_next (&child_iter);

      {
	GConfValue *val;

	val = gconf_value_decode (tmp);

	if (val)
	  gconf_schema_set_default_value_nocopy (schema, val);
      }

      dbus_free (tmp);
    }
  else if (strcmp (name, "car") == 0)
    {
      value = gconf_value_new (GCONF_VALUE_PAIR);

      gconf_value_set_car_nocopy (value, gconf_value_from_dict (dict, "car"));
      gconf_value_set_cdr_nocopy (value, gconf_value_from_dict (dict, "cdr"));
    }

  dbus_free (name);
  
  return value;
}

void
gconf_dbus_message_append_gconf_value (DBusMessage      *message,
				       const GConfValue *value)
{
  DBusMessageIter iter;
  
  dbus_message_append_iter_init (message, &iter);
  
  if (!value)
    {
      dbus_message_iter_append_nil (&iter);
      return;
    }

  switch (value->type)
    {
    case GCONF_VALUE_INT:
      dbus_message_iter_append_int32 (&iter, gconf_value_get_int (value));
      break;
    case GCONF_VALUE_STRING:
      dbus_message_iter_append_string (&iter, gconf_value_get_string (value));
      break;
    case GCONF_VALUE_FLOAT:
      dbus_message_iter_append_double (&iter, gconf_value_get_float (value));
      break;
    case GCONF_VALUE_BOOL:
      dbus_message_iter_append_boolean (&iter, gconf_value_get_bool (value));
      break;
    case GCONF_VALUE_INVALID:
      dbus_message_iter_append_nil (&iter);
      break;
    case GCONF_VALUE_LIST:
      {
	guint i, len;
        GSList* list;

        list = gconf_value_get_list (value);
        len = g_slist_length (list);

	switch (gconf_value_get_list_type (value))
	  {
	  case GCONF_VALUE_STRING:
	    {
	      const char **str;

	      str = (const char **)g_new (char *, len + 1);
	      str[len] = NULL;

	      i = 0;
	      while (list)
		{
		  GConfValue *value = list->data;

		  str[i] = (gchar *)gconf_value_get_string (value);
		    
		  list = list->next;
		  ++i;
		}

	      dbus_message_iter_append_string_array (&iter, str, len);
	      g_free (str);
	      break;
	    }
	  case GCONF_VALUE_INT:
	    {
	      int *array;

	      array = g_new (dbus_int32_t, len);

	      i = 0;
	      while (list)
		{
		  GConfValue *value = list->data;
		  
		  array[i] = gconf_value_get_int (value);
		    
		  list = list->next;
		  ++i;
		}

	      dbus_message_iter_append_int32_array (&iter, array, len);
	      g_free (array);
	      break;
	    }
	  case GCONF_VALUE_FLOAT:
	    {
	      double *array;

	      array = g_new (double, len);

	      i = 0;
	      while (list)
		{
		  GConfValue *value = list->data;
		  
		  array[i] = gconf_value_get_float (value);
		    
		  list = list->next;
		  ++i;
		}

	      dbus_message_iter_append_double_array (&iter, array, len);
	      g_free (array);
	      break;
	    }
	  case GCONF_VALUE_BOOL:
	    {
	      unsigned char *array;

	      array = g_new (unsigned char, len);

	      i = 0;
	      while (list)
		{
		  GConfValue *value = list->data;
		  
		  array[i] = gconf_value_get_bool (value);
		    
		  list = list->next;
		  ++i;
		}

	      dbus_message_iter_append_boolean_array (&iter, array, len);
	      g_free (array);
	      break;
	    }
	  default:
	    g_error ("unsupported gconf list value type %d", gconf_value_get_list_type (value));
	  }
	break;	
      }
    case GCONF_VALUE_PAIR:
      {
	DBusMessageIter dict;
	
	dbus_message_iter_append_dict (&iter, &dict);

	set_dict_value_from_gconf_value (&dict, "car", gconf_value_get_car (value));
	set_dict_value_from_gconf_value (&dict, "cdr", gconf_value_get_cdr (value));

	break;
      }
    case GCONF_VALUE_SCHEMA:
      gconf_dbus_message_append_gconf_schema (message, gconf_value_get_schema (value));
      break;

    default:
      g_error ("unsupported gconf value type %d", value->type);
    }
}

GConfValue *
gconf_dbus_create_gconf_value_from_message (DBusMessageIter *iter)
{
  int arg_type;
  GConfValue *gval;
  GConfValueType type = GCONF_VALUE_INVALID;
  
  arg_type = dbus_message_iter_get_arg_type (iter);

  switch (arg_type)
    {
    case DBUS_TYPE_NIL:
      return NULL;
    case DBUS_TYPE_BOOLEAN:
      type = GCONF_VALUE_BOOL;
      break;
    case DBUS_TYPE_INT32:
      type = GCONF_VALUE_INT;
      break;
    case DBUS_TYPE_DOUBLE:
      type = GCONF_VALUE_FLOAT;
      break;
    case DBUS_TYPE_STRING:
      type = GCONF_VALUE_STRING;
      break;
    case DBUS_TYPE_ARRAY:
      type = GCONF_VALUE_LIST;
      break;
    case DBUS_TYPE_DICT:
      return gconf_dbus_create_gconf_value_from_dict (iter);
      break;
    default:
      g_error ("unsupported arg type %d\n", arg_type);
    }

  g_assert (GCONF_VALUE_TYPE_VALID (type));

  gval = gconf_value_new (type);

  switch (gval->type)
    {
    case GCONF_VALUE_BOOL:
      gconf_value_set_bool (gval, dbus_message_iter_get_boolean (iter));
      break;
    case GCONF_VALUE_INT:
      gconf_value_set_int (gval, dbus_message_iter_get_int32 (iter));
      break;
    case GCONF_VALUE_FLOAT:
      gconf_value_set_float (gval, dbus_message_iter_get_double (iter));
      break;
    case GCONF_VALUE_STRING:
      {
	char *str;
	str = dbus_message_iter_get_string (iter);

	gconf_value_set_string (gval, str);
	dbus_free (str);
	break;
      }
    case GCONF_VALUE_LIST:
      {
	GSList *list = NULL;
	guint i = 0;

	switch (dbus_message_iter_get_array_type (iter))
	  {
	  case DBUS_TYPE_BOOLEAN:
	    {
	      unsigned char *array;
	      int len;

	      dbus_message_iter_get_boolean_array (iter, &array, &len);

	      for (i = 0; i < len; i++)
		{
		  GConfValue *value = gconf_value_new (GCONF_VALUE_BOOL);
		  
		  gconf_value_set_bool (value, array[i]);
		  
		  list = g_slist_prepend (list, value);
		}
	      list = g_slist_reverse (list);
	      dbus_free (array);

	      gconf_value_set_list_type (gval, GCONF_VALUE_BOOL);
	      gconf_value_set_list_nocopy (gval, list);
	      break;
	    }
	  case DBUS_TYPE_DOUBLE:
	    {
	      double *array;
	      int len;

	      dbus_message_iter_get_double_array (iter, &array, &len);

	      for (i = 0; i < len; i++)
		{
		  GConfValue *value = gconf_value_new (GCONF_VALUE_FLOAT);
		  
		  gconf_value_set_float (value, array[i]);
		  
		  list = g_slist_prepend (list, value);
		}
	      list = g_slist_reverse (list);
	      dbus_free (array);

	      gconf_value_set_list_type (gval, GCONF_VALUE_FLOAT);
	      gconf_value_set_list_nocopy (gval, list);
	      break;
	    }
	  case DBUS_TYPE_INT32:
	    {
	      dbus_int32_t *array;
	      int len;

	      dbus_message_iter_get_int32_array (iter, &array, &len);

	      for (i = 0; i < len; i++)
		{
		  GConfValue *value = gconf_value_new (GCONF_VALUE_INT);
		  
		  gconf_value_set_int (value, array[i]);
		  
		  list = g_slist_prepend (list, value);
		}
	      list = g_slist_reverse (list);
	      dbus_free (array);

	      gconf_value_set_list_type (gval, GCONF_VALUE_INT);
	      gconf_value_set_list_nocopy (gval, list);
	      break;
	    }
	  case DBUS_TYPE_STRING:
	    {
	      char **array;
	      int len;
	      
	      dbus_message_iter_get_string_array (iter, &array, &len);
	      
	      for (i = 0; i < len; i++)
		{
		  GConfValue *value = gconf_value_new (GCONF_VALUE_STRING);
		  
		  gconf_value_set_string (value, array[i]);
		  
		  list = g_slist_prepend (list, value);
		}
	      list = g_slist_reverse (list);
	      dbus_free_string_array (array);

	      gconf_value_set_list_type (gval, GCONF_VALUE_STRING);
	      gconf_value_set_list_nocopy (gval, list);
	      break;
	    }
	  default:
	    g_error ("unknown list arg type %d", arg_type);
	  }
	break;
      }
    default:
      g_assert_not_reached ();
      break;
    }

  return gval;
}

/*
Set    (Array<string> keys,
        v1, v2, v3, ...);
*/

/* FIXME: what's the arguments going to be here? */
void
set (DBusConnection *conn, GList *entries)
{
#if 0
  DBusMessage *message, *reply;
  DBusError error;
  GList *list;
  const char **str;
  
  message = dbus_message_new (GCONF_SERVICE_NAME, FUNC_DB_SET);

  /* Create array<string> with the keys. */
  str = (const char **)g_new (char *, len + 1);
  str[len] = NULL;
	
  i = 0;
  list = entries;
  while (list)
    {
      GConfValue *value = gconf_entry_get_value (list->data);
      
      str[i] = (gchar *) "name";
      
      list = list->next;
      ++i;
    }
  
  dbus_message_iter_append_string_array (&iter, str, len);
  g_free (str);

  list = entries;
  while (list)
    {
      GConfValue *value = gconf_entry_get_value (list->data);
      
      gconf_dbus_message_append_gconf_value (message, value);
      
      list = list->next;
    }


  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (dbus_conn, message, -1, &error);
  dbus_message_unref (message);

  if (gconf_handle_dbus_exception(reply, &error, err))
    {
      if (reply)
	dbus_message_unref (reply);
      return NULL;
    }
  
  dbus_message_unref (reply);
#endif
}