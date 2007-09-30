/* 
 * Copyright 2006, 2007 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * Libxr is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libxr.  If not, see <http://www.gnu.org/licenses/>.
 */

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

/* get property from xml node */
static __inline__ gchar* xml_get_prop_str(xmlNodePtr n, const gchar* name)
{
  xmlChar* prop = xmlGetProp(n, BAD_CAST name);
  if (prop == NULL)
    return NULL;
  gchar* dup = g_strdup((gchar*)prop);
  xmlFree(prop);
  return dup;
}

static __inline__ int xml_get_prop_int(xmlNodePtr n, const gchar* name)
{
  xmlChar* prop = xmlGetProp(n, BAD_CAST name);
  if (prop == NULL)
    return -1;
  int v = atoi((const char*)prop);
  xmlFree(prop);
  return v;
}

static __inline__ char* xml_get_cont_str(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == NULL)
    return NULL;
  char* dup = g_strdup((char*)str);
  xmlFree(str);
  return dup;
}

static __inline__ int xml_get_cont_int(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == NULL)
    return 0;
  int dup = atoi((const char*)str);
  xmlFree(str);
  return dup;
}

static __inline__ double xml_get_cont_double(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == NULL)
    return 0.0;
  double v = atof((const char*)str);
  xmlFree(str);
  return v;
}

static __inline__ int xml_get_cont_bool(xmlNodePtr n)
{
  xmlChar* str = xmlNodeListGetString(n->doc, n->xmlChildrenNode, 1);
  if (str == NULL)
    return 0;
  int retval = -1;
  if (!strcmp(str, "0"))
    retval = 0;
  if (!strcmp(str, "1"))
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
  if (ns->obj == NULL)
    xmlXPathFreeObject(o);
  return ns;
}

static __inline__ void xp_free_nodes(struct nodeset* ns)
{
  if (ns == NULL)
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
  { \
    xmlNodePtr child; \
    for (child = parent->xmlChildrenNode; child != NULL; child = child->next) \
    {

#define for_each_node_end() }}

#define match_node(_node, _name) \
  (_node->type == XML_ELEMENT_NODE && !strcmp((gchar*)_node->name, _name))

#define match_comment(_node) \
  (_node->type == XML_COMMENT_NODE)

#endif
