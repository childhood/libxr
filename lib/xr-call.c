#include <string.h>

#include "xml-priv.h"
#include "xr-call.h"

xr_call* xr_call_new(char* method)
{
  xr_call* c = g_new0(xr_call, 1);
  c->method = g_strdup(method);
  return c;
}

void xr_call_add_param(xr_call* call, xr_value* val)
{
  g_assert(call != NULL);
  g_assert(val != NULL);
  call->params = g_slist_append(call->params, val);
}

void xr_call_set_retval(xr_call* call, xr_value* val)
{
  g_assert(call != NULL);
  g_assert(val != NULL);
  call->retval = val;
}

void xr_call_set_errval(xr_call* call, int code, char* msg)
{
  g_assert(call != NULL);
  g_assert(msg != NULL);
  call->errval = xr_value_struct_new();
  xr_value_struct_set_member(call->errval, "faultCode", xr_value_int_new(code));
  xr_value_struct_set_member(call->errval, "faultString", xr_value_string_new(msg));
}

static void _xr_value_serialize(xmlNode* node, xr_value* val)
{
  xmlNode* value;
  char* data;
  char buf[50];
  GSList* i;
  if (val->type != XRV_MEMBER)
    node = xmlNewChild(node, NULL, BAD_CAST "value", NULL);
  switch (val->type)
  {
    case XRV_ARRAY:
      value = xmlNewChild(node, NULL, BAD_CAST "array", NULL);
      for (i = val->children; i; i = i->next)
        _xr_value_serialize(value, i->data);
      break;
    case XRV_STRUCT:
      value = xmlNewChild(node, NULL, BAD_CAST "struct", NULL);
      for (i = val->children; i; i = i->next)
        _xr_value_serialize(value, i->data);
      break;
    case XRV_MEMBER:
      value = xmlNewChild(node, NULL, BAD_CAST "member", NULL);
      xmlNewChild(value, NULL, BAD_CAST "name", BAD_CAST val->member_name);
      _xr_value_serialize(value, val->member_value);
      break;
    case XRV_INT:
      snprintf(buf, sizeof(buf), "%d", val->int_val);
      value = xmlNewChild(node, NULL, BAD_CAST "int", BAD_CAST buf);
      break;
    case XRV_STRING:
      value = xmlNewChild(node, NULL, BAD_CAST "string", BAD_CAST val->str_val);
      break;
    case XRV_BOOLEAN:
      value = xmlNewChild(node, NULL, BAD_CAST "boolean", BAD_CAST (val->int_val ? "true" : "false"));
      break;
    case XRV_DOUBLE:
      snprintf(buf, sizeof(buf), "%g", val->dbl_val);
      value = xmlNewChild(node, NULL, BAD_CAST "double", NULL);
      break;
    case XRV_TIME:
      value = xmlNewChild(node, NULL, BAD_CAST "time", BAD_CAST val->str_val);
      break;
    case XRV_BLOB:
      //XXX: not exactly efficient
      data = g_base64_encode(val->str_val, val->buf_len);
      value = xmlNewChild(node, NULL, BAD_CAST "base64", BAD_CAST data);
      g_free(data);
      break;
  }
}

int xr_call_serialize_request(xr_call* call, char** buf, int* len)
{
  GSList* i;
  g_assert(call != NULL);
  xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode* root = xmlNewNode(NULL, BAD_CAST "methodCall");
  xmlDocSetRootElement(doc, root);
  xmlNewChild(root, NULL, BAD_CAST "methodName", call->method);
  xmlNode* params = xmlNewChild(root, NULL, BAD_CAST "params", NULL);
  for (i = call->params; i; i = i->next)
  {
    xmlNode* param = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
    _xr_value_serialize(param, i->data);
  }
  xmlDocDumpFormatMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8", 1);
  return 0;
}

void xr_call_free_buffer(char* buf)
{
  xmlFree(buf);
}

int xr_call_serialize_response(xr_call* call, char** buf, int* len)
{
  g_assert(call != NULL);
  g_assert(call->errval != NULL || call->retval != NULL);
  xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode* root = xmlNewNode(NULL, BAD_CAST "methodResponse");
  xmlDocSetRootElement(doc, root);
  xmlNode* params = xmlNewChild(root, NULL, BAD_CAST "params", NULL);
  xmlNode* param = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
  if (call->errval)
    _xr_value_serialize(param, call->errval);
  else if (call->retval)
    _xr_value_serialize(param, call->retval);
  xmlDocDumpFormatMemoryEnc(doc, (xmlChar**)buf, len, "UTF-8", 1);
  return 0;
}

static xr_value* _xr_value_unserialize(xmlNode* node)
{
  for_each_node(node, tn)
  {
    if (match_node(tn, "int"))
      return xr_value_int_new(xml_get_cont_int(tn));
    else if (match_node(tn, "string"))
      return xr_value_string_new(xml_get_cont_str(tn));
    else if (match_node(tn, "boolean"))
      return xr_value_bool_new(xml_get_cont_bool(tn));
    else if (match_node(tn, "double"))
      return xr_value_double_new(0);
    else if (match_node(tn, "time"))
      return xr_value_time_new(xml_get_cont_str(tn));
    else if (match_node(tn, "base64"))
    {
      char* base64 = xml_get_cont_str(tn);
      gsize len = 0;
      char* buf = g_base64_decode(base64, &len);
      g_free(base64);
      return xr_value_blob_new(buf, len);
    }
    else if (match_node(tn, "array"))
    {
      xr_value* arr = xr_value_array_new();
      for_each_node(tn, v)
      {
        if (match_node(v, "value"))
        {
          xr_value* elem = _xr_value_unserialize(v);
          if (elem)
            xr_value_array_append(arr, elem);
        }
      }
      return arr;
    }
    else if (match_node(tn, "struct"))
    {
      xr_value* str = xr_value_struct_new();
      for_each_node(tn, m)
      {
        if (match_node(m, "member"))
        {
          char* name = NULL;
          xr_value* val = NULL;
          int names = 0, values = 0;

          for_each_node(m, me)
          {
            if (match_node(me, "name"))
            {
              if (names++ == 0)
                name = xml_get_cont_str(me);
            }
            else if (match_node(me, "value"))
            {
              if (values++ == 0)
                val = _xr_value_unserialize(me);
            }
          }
          if (values != 1 || names != 1)
          {
            g_free(name);
            xr_value_free(val);
          }
          xr_value_struct_set_member(str, name, val);
        }
      }
      return str;
    }
  }
  return NULL;
}

int xr_call_unserialize_request(xr_call* call, char* buf, int len)
{
  g_assert(call != NULL);
  g_assert(buf != NULL);
  g_assert(len > 0);
  xmlDoc* doc = xmlReadMemory(buf, len, 0, 0, XML_PARSE_NOWARNING|XML_PARSE_NOERROR|XML_PARSE_NONET);
  if (doc == NULL)
    goto err_0;
  xmlNode* root = xmlDocGetRootElement(doc);
  if (root == NULL)
    goto err_1;
  xmlXPathContext* ctx = xmlXPathNewContext(doc);
  call->method = xp_eval_cont_str(ctx, "/methodCall/methodName");
  struct nodeset* ns = xp_eval_nodes(ctx, "/methodCall/params/param/value[0]");
  for (int i = 0; i < ns->count; i++)
    xr_call_add_param(call, _xr_value_unserialize(ns->nodes[i]));
  xp_free_nodes(ns);
  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);
  return 0;
 err_1:
  xmlFreeDoc(doc);
 err_0:
  return -1;
}

int xr_call_unserialize_response(xr_call* call, char* buf, int len)
{
  g_assert(call != NULL);
  g_assert(buf != NULL);
  g_assert(len > 0);
  xmlDoc* doc = xmlReadMemory(buf, len, 0, 0, XML_PARSE_NOWARNING|XML_PARSE_NOERROR|XML_PARSE_NONET);
  if (doc == NULL)
    goto err_0;
  xmlNode* root = xmlDocGetRootElement(doc);
  if (root == NULL)
    goto err_1;
  xmlXPathContext* ctx = xmlXPathNewContext(doc);
  struct nodeset* ns = xp_eval_nodes(ctx, "/methodResponse/params/param[0]/value[0]");
  for (int i = 0; i < ns->count; i++)
    call->retval = _xr_value_unserialize(ns->nodes[i]);
  xp_free_nodes(ns);
  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);
  return 0;
 err_1:
  xmlFreeDoc(doc);
 err_0:
  return -1;
}

void xr_call_free(xr_call* call)
{
  GSList* i;
  g_free(call->method);
  for (i = call->params; i; i = i->next)
    xr_value_free(i->data);
  xr_value_free(call->retval);
  xr_value_free(call->errval);
  g_free(call);
}
