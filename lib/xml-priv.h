#ifndef __XML_HELPERS_API_H
#define __XML_HELPERS_API_H

#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <unistd.h>
#include <string.h>

/* this file contains frequently used macros and functions,
 for xml parsing/generation */

/* load xml file */
static __inline__ gint xml_load_doc(const gchar* file, xmlDocPtr* d, xmlNodePtr* r)
{
  if (access(file, R_OK) < 0)
    return 1;
  *d = xmlReadFile(file, 0, XML_PARSE_NOWARNING|XML_PARSE_NOERROR|XML_PARSE_NONET);
  if (*d == 0)
    return 1;
  *r = xmlDocGetRootElement(*d);
  if (*r == 0)
  {
    xmlFreeDoc(*d);
    return 1;
  }
  return 0;
}

#define xml_load_doc_or(file) \
  xmlDocPtr d=0; xmlNodePtr r=0; \
  if (xml_load_doc(file, &d, &r))

static __inline__ void xml_set_prop(xmlNodePtr n, const gchar* name, const gchar* value)
{
  xmlSetProp(n, BAD_CAST name, BAD_CAST value);
}

static __inline__ void xml_set_cont(xmlNodePtr n, const gchar* cont)
{
  xmlNodeSetContent(n, BAD_CAST cont);
}

/* get property from xml node */
static __inline__ gchar* xml_get_prop_str(xmlNodePtr n, const gchar* name)
{
  xmlChar* prop = xmlGetProp(n, BAD_CAST name);
  if (prop == 0)
    return 0;
  gchar* dup = g_strdup((gchar*)prop);
  xmlFree(prop);
  return dup;
}

static __inline__ int xml_get_prop_int(xmlNodePtr n, const gchar* name)
{
  xmlChar* prop = xmlGetProp(n, BAD_CAST name);
  if (prop == 0)
    return -1;
  int v = atoi((const char*)prop);
  xmlFree(prop);
  return v;
}

static __inline__ char* xml_get_cont_str(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == 0)
    return 0;
  char* dup = g_strdup((char*)str);
  xmlFree(str);
  return dup;
}

static __inline__ int xml_get_cont_int(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == 0)
    return 0;
  int dup = atoi((const char*)str);
  xmlFree(str);
  return dup;
}

static __inline__ double xml_get_cont_double(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == 0)
    return 0;
  double v = atof((const char*)str);
  xmlFree(str);
  return v;
}

static __inline__ int xml_get_cont_bool(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == 0)
    return 0;
  int retval = -1;
  if (!strcmp(str, "false"))
    retval = 0;
  if (!strcmp(str, "true"))
    retval = 1;
  xmlFree(str);
  return retval;
}

/* easy xpath wrapper */

struct nodeset
{
  xmlXPathObjectPtr obj;
  xmlNodePtr* nodes;
  long count;
};

static __inline__ struct nodeset* xp_eval_nodes(xmlXPathContextPtr ctx, const gchar* path)
{
  struct nodeset* ns = g_new0(struct nodeset, 1);
  xmlXPathObjectPtr o = xmlXPathEvalExpression(path, ctx);
  if (o && o->type == XPATH_NODESET && o->nodesetval)
  {
    ns->obj = o;
    ns->nodes = o->nodesetval->nodeTab;
    ns->count = o->nodesetval->nodeNr;
  }
  if (!ns->obj)
    xmlXPathFreeObject(o);
  return ns;
}

static __inline__ void xp_free_nodes(struct nodeset* ns)
{
  if (ns == 0)
    return;
  if (ns->obj)
    xmlXPathFreeObject(ns->obj);
  g_free(ns);
}

static __inline__ gchar* xp_eval_cont_str(xmlXPathContextPtr ctx, const gchar* path)
{
  gchar* ret = NULL;
  xmlXPathObjectPtr o = xmlXPathEvalExpression(path, ctx);
  if (o && o->type == XPATH_NODESET && o->nodesetval && o->nodesetval->nodeNr > 0)
    ret = xml_get_cont_str(o->nodesetval->nodeTab[0]);
  if (o)
    xmlXPathFreeObject(o);
  return ret;
}

static __inline__ gint xp_eval_cont_int(xmlXPathContextPtr ctx, const gchar* path, gint fallback)
{
  gint ret = fallback;
  xmlXPathObjectPtr o = xmlXPathEvalExpression(path, ctx);
  if (o && o->type == XPATH_NODESET && o->nodesetval && o->nodesetval->nodeNr > 0)
    ret = xml_get_cont_int(o->nodesetval->nodeTab[0]);
  if (o)
    xmlXPathFreeObject(o);
  return ret;
}

#define for_each_node(parent, child) \
  for (xmlNodePtr child = parent->xmlChildrenNode; child != 0; child = child->next)

#define match_node(_node, _name) \
  (_node->type == XML_ELEMENT_NODE && !strcmp((gchar*)_node->name, _name))

#define match_comment(_node) \
  (_node->type == XML_COMMENT_NODE)

#endif
