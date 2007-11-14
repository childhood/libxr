#include "xml-priv.h"

static void _xr_value_serialize_xmlrpc(xmlNode* node, xr_value* val)
{
  xmlNode* value;
  char buf[50];
  GSList* i;

  if (xr_value_get_type(val) != XRV_MEMBER)
    node = xmlNewChild(node, NULL, BAD_CAST "value", NULL);

  switch (xr_value_get_type(val))
  {
    case XRV_ARRAY:
    {
      value = xmlNewChild(node, NULL, BAD_CAST "array", NULL);
      value = xmlNewChild(value, NULL, BAD_CAST "data", NULL);
      for (i = xr_value_get_items(val); i; i = i->next)
        _xr_value_serialize_xmlrpc(value, i->data);
      break;
    }
    case XRV_STRUCT:
    {
      value = xmlNewChild(node, NULL, BAD_CAST "struct", NULL);
      for (i = xr_value_get_members(val); i; i = i->next)
        _xr_value_serialize_xmlrpc(value, i->data);
      break;
    }
    case XRV_MEMBER:
    {
      value = xmlNewChild(node, NULL, BAD_CAST "member", NULL);
      xmlNewChild(value, NULL, BAD_CAST "name", BAD_CAST xr_value_get_member_name(val));
      _xr_value_serialize_xmlrpc(value, xr_value_get_member_value(val));
      break;
    }
    case XRV_INT:
    {
      int int_val = -1;
      xr_value_to_int(val, &int_val);
      snprintf(buf, sizeof(buf), "%d", int_val);
      value = xmlNewChild(node, NULL, BAD_CAST "int", BAD_CAST buf);
      break;
    }
    case XRV_STRING:
    {
      char* str_val = NULL;
      xr_value_to_string(val, &str_val);
      value = xmlNewChild(node, NULL, BAD_CAST "string", BAD_CAST str_val);
      g_free(str_val);
      break;
    }
    case XRV_BOOLEAN:
    {
      int bool_val = -1;
      xr_value_to_bool(val, &bool_val);
      value = xmlNewChild(node, NULL, BAD_CAST "boolean", BAD_CAST (bool_val ? "1" : "0"));
      break;
    }
    case XRV_DOUBLE:
    {
      double dbl_val = -1;
      xr_value_to_double(val, &dbl_val);
      snprintf(buf, sizeof(buf), "%g", dbl_val);
      value = xmlNewChild(node, NULL, BAD_CAST "double", NULL);
      break;
    }
    case XRV_TIME:
    {
      char* str_val = NULL;
      xr_value_to_time(val, &str_val);
      value = xmlNewChild(node, NULL, BAD_CAST "dateTime.iso8601", BAD_CAST str_val);
      g_free(str_val);
      break;
    }
    case XRV_BLOB:
    {
      char* data = NULL;
      xr_blob* b = NULL;
      xr_value_to_blob(val, &b);
      data = g_base64_encode(b->buf, b->len);
      xr_blob_unref(b);
      value = xmlNewChild(node, NULL, BAD_CAST "base64", BAD_CAST data);
      g_free(data);
      break;
    }
  }
}

static xr_value* _xr_value_unserialize_xmlrpc(xmlNode* node)
{
  for_each_node(node, tn)
    if (match_node(tn, "int") || match_node(tn, "i4"))
      return xr_value_int_new(xml_get_cont_int(tn));
    else if (match_node(tn, "string"))
    {
      char* str = xml_get_cont_str(tn);
      xr_value* val = xr_value_string_new(str);
      g_free(str);
      return val;
    }
    else if (match_node(tn, "boolean"))
      return xr_value_bool_new(xml_get_cont_bool(tn));
    else if (match_node(tn, "double"))
      return xr_value_double_new(xml_get_cont_double(tn));
    else if (match_node(tn, "dateTime.iso8601"))
    {
      char* str = xml_get_cont_str(tn);
      xr_value* val = xr_value_time_new(str);
      g_free(str);
      return val;
    }
    else if (match_node(tn, "base64"))
    {
      xr_blob* b;
      xr_value* bv;
      char* base64 = xml_get_cont_str(tn);
      gsize len = 0;
      char* buf = g_base64_decode(base64, &len);
      g_free(base64);
      b = xr_blob_new(buf, len);
      bv = xr_value_blob_new(b);
      xr_blob_unref(b);
      return bv;
    }
    else if (match_node(tn, "array"))
    {
      xr_value* arr = xr_value_array_new();
      for_each_node(tn, d)
        if (match_node(d, "data"))
        {
          for_each_node(d, v)
            if (match_node(v, "value"))
            {
              xr_value* elem = _xr_value_unserialize_xmlrpc(v);
              if (elem == NULL)
              {
                xr_value_unref(arr);
                return NULL;
              }
              xr_value_array_append(arr, elem);
            }
          for_each_node_end()
          return arr;
        }
      for_each_node_end()
      return arr;
    }
    else if (match_node(tn, "struct"))
    {
      xr_value* str = xr_value_struct_new();
      for_each_node(tn, m)
        if (match_node(m, "member"))
        {
          char* name = NULL;
          xr_value* val = NULL;
          int names = 0, values = 0;

          for_each_node(m, me)
            if (match_node(me, "name"))
            {
              if (names++ == 0)
                name = xml_get_cont_str(me);
            }
            else if (match_node(me, "value"))
            {
              if (values++ == 0)
                val = _xr_value_unserialize_xmlrpc(me);
            }
          for_each_node_end()

          if (values != 1 || names != 1)
          {
            g_free(name);
            xr_value_unref(val);
            xr_value_unref(str);
            return NULL;
          }
          xr_value_struct_set_member(str, name, val);
          g_free(name);
        }
      for_each_node_end()
      return str;
    }
  for_each_node_end()
  return NULL;
}

static void xr_call_serialize_request_xmlrpc(xr_call* call, char** buf, int* len)
{
  xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode* root = xmlNewNode(NULL, BAD_CAST "methodCall");
  xmlDocSetRootElement(doc, root);
  xmlNewChild(root, NULL, BAD_CAST "methodName", call->method);
  xmlNode* params = xmlNewChild(root, NULL, BAD_CAST "params", NULL);

  GSList* i;
  for (i = call->params; i; i = i->next)
  {
    xmlNode* param = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
    _xr_value_serialize_xmlrpc(param, i->data);
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    xmlDocDumpFormatMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8", 1);
  else
    xmlDocDumpMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8");

  xmlFreeDoc(doc);
}

static void xr_call_serialize_response_xmlrpc(xr_call* call, char** buf, int* len)
{
  xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode* root = xmlNewNode(NULL, BAD_CAST "methodResponse");
  xmlDocSetRootElement(doc, root);

  if (call->error_set)
  {
    xmlNode* fault = xmlNewChild(root, NULL, BAD_CAST "fault", NULL);
    xr_value* v = xr_value_struct_new();
    xr_value_struct_set_member(v, "faultCode", xr_value_int_new(call->errcode));
    xr_value_struct_set_member(v, "faultString", xr_value_string_new(call->errmsg));
    _xr_value_serialize_xmlrpc(fault, v);
    xr_value_unref(v);
  }
  else if (call->retval)
  {
    xmlNode* params = xmlNewChild(root, NULL, BAD_CAST "params", NULL);
    xmlNode* param = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
    _xr_value_serialize_xmlrpc(param, call->retval);
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    xmlDocDumpFormatMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8", 1);
  else
    xmlDocDumpMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8");

  xmlFreeDoc(doc);
}

static gboolean xr_call_unserialize_request_xmlrpc(xr_call* call, const char* buf, int len)
{
  xmlDoc* doc = xmlReadMemory(buf, len, 0, 0, XML_PARSE_NOWARNING|XML_PARSE_NOERROR|XML_PARSE_NONET);
  if (doc == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML request. Invalid XML document.");
    goto err_0;
  }

  xmlNode* root = xmlDocGetRootElement(doc);
  if (root == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML request. Root element is missing.");
    goto err_1;
  }

  xmlXPathContext* ctx = xmlXPathNewContext(doc);

  call->method = xp_eval_cont_str(ctx, "/methodCall/methodName");
  if (call->method == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML request. Missing methodName.");
    goto err_2;
  }

  int i;
  struct nodeset* ns = xp_eval_nodes(ctx, "/methodCall/params/param/value");
  for (i = 0; i < ns->count; i++)
  {
    xr_value* v = _xr_value_unserialize_xmlrpc(ns->nodes[i]);
    if (v == NULL)
    {
      xr_call_set_error(call, -1, "Can't parse XML-RPC XML request. Failed to unserialize parameter %d.", i);
      goto err_3;
    }
    xr_call_add_param(call, v);
  }
  xp_free_nodes(ns);

  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);
  return TRUE;

 err_3:
  xp_free_nodes(ns);
 err_2:
  xmlXPathFreeContext(ctx);
 err_1:
  xmlFreeDoc(doc);
 err_0:
  return FALSE;
}

static gboolean xr_call_unserialize_response_xmlrpc(xr_call* call, const char* buf, int len)
{
  xmlDoc* doc = xmlReadMemory(buf, len, 0, 0, XML_PARSE_NOWARNING|XML_PARSE_NOERROR|XML_PARSE_NONET);
  if (doc == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Invalid XML document.");
    goto err_0;
  }

  xmlNode* root = xmlDocGetRootElement(doc);
  if (root == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Root element is missing.");
    goto err_1;
  }

  xmlXPathContext* ctx = xmlXPathNewContext(doc);

  struct nodeset* ns = xp_eval_nodes(ctx, "/methodResponse/params/param/value");
  if (ns->count == 1)
  {
    call->retval = _xr_value_unserialize_xmlrpc(ns->nodes[0]);
    if (call->retval == NULL)
    {
      xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Failed to unserialize retval.");
      goto err_2;
    }
    goto done;
  }
  else if (ns->count > 1) // more than one param is bad
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Too many return values.");
    goto err_2;
  }
  xp_free_nodes(ns);

  // ok no params/param, check for fault
  ns = xp_eval_nodes(ctx, "/methodResponse/fault/value");
  if (ns->count == 1)
  {
    call->retval = _xr_value_unserialize_xmlrpc(ns->nodes[0]);
    if (call->retval == NULL)
    {
      xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Failed to unserialize fault response.");
      goto err_2;
    }
    // check if client returned standard XML-RPC error message, we want to process
    // it differently than normal retval
    int errcode = 0;
    char* errmsg = NULL;
    if (xr_value_is_error_retval(call->retval, &errcode, &errmsg))
    {
      xr_call_set_error(call, errcode, "%s", errmsg);
      g_free(errmsg);
      goto err_3;
    }
    else
    {
      xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Invalid fault response.");
      goto err_3;
    }
  }
  else // no fault either
  {
    xr_call_set_error(call, -1, "Can't parse XML-RPC XML response. Failed to unserialize retval.");
    goto err_2;
  }

done:
  xp_free_nodes(ns);
  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);
  return TRUE;

 err_3:
  xr_value_unref(call->retval);
  call->retval = NULL;
 err_2:
  xp_free_nodes(ns);
  xmlXPathFreeContext(ctx);
 err_1:
  xmlFreeDoc(doc);
 err_0:
  return FALSE;
}

static void xr_call_free_buffer_xmlrpc(xr_call* call, char* buf)
{
  xmlFree(buf);
}
