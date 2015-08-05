/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include "mod_mysqlx_session.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_view.h"
#include "mod_mysqlx_resultset.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Schema::Schema(boost::shared_ptr<ApiBaseSession> session, const std::string &schema)
: DatabaseObject(session, boost::shared_ptr<DatabaseObject>(), schema), _schema_impl(session->session_obj()->getSchema(schema))
{
  init();
}

Schema::Schema(boost::shared_ptr<const ApiBaseSession> session, const std::string &schema) :
DatabaseObject(boost::const_pointer_cast<ApiBaseSession>(session), boost::shared_ptr<DatabaseObject>(), schema)
{
  init();
}

Schema::~Schema()
{
}

void Schema::init()
{
  add_method("getTables", boost::bind(&DatabaseObject::get_member_method, this, _1, "getTables", "tables"), "name", shcore::String, NULL);
  add_method("getCollections", boost::bind(&DatabaseObject::get_member_method, this, _1, "getCollections", "collections"), "name", shcore::String, NULL);
  add_method("getViews", boost::bind(&DatabaseObject::get_member_method, this, _1, "getViews", "views"), "name", shcore::String, NULL);

  add_method("getTable", boost::bind(&Schema::getTable, this, _1), "name", shcore::String, NULL);
  add_method("getCollection", boost::bind(&Schema::getCollection, this, _1), "name", shcore::String, NULL);
  add_method("getView", boost::bind(&Schema::getView, this, _1), "name", shcore::String, NULL);

  add_method("createCollection", boost::bind(&Schema::createCollection, this, _1), "name", shcore::String, NULL);

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
  _collections = Value::new_map().as_map();
}

void Schema::cache_table_objects()
{
  try
  {
    boost::shared_ptr<ApiBaseSession> sess(boost::static_pointer_cast<ApiBaseSession>(_session.lock()));
    if (sess)
    {
      {
        sess->flush_last_result();

        shcore::Argument_list args;
        args.push_back(Value(_name));
        args.push_back(Value(""));

        Value myres = sess->executeAdminCommand("list_objects", args);
        boost::shared_ptr<mysh::mysqlx::Resultset> my_res = myres.as_object<mysh::mysqlx::Resultset>();

        Value raw_entry;

        while ((raw_entry = my_res->next(shcore::Argument_list())))
        {
          boost::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();
          std::string object_name = row->get_member("name").as_string();
          std::string object_type = row->get_member("type").as_string();

          if (object_type == "TABLE")
          {
            boost::shared_ptr<Table> table(new Table(shared_from_this(), object_name));
            (*_tables)[object_name] = Value(boost::static_pointer_cast<Object_bridge>(table));
          }
          else if (object_type == "VIEW")
          {
            boost::shared_ptr<View> view(new View(shared_from_this(), object_name));
            (*_views)[object_name] = Value(boost::static_pointer_cast<Object_bridge>(view));
          }
          else if (object_type == "COLLECTION")
          {
            boost::shared_ptr<Collection> collection(new Collection(shared_from_this(), object_name));
            (*_collections)[object_name] = Value(boost::static_pointer_cast<Object_bridge>(collection));
          }
          else
            throw shcore::Exception::logic_error((boost::format("Unexpected object type returned from server, object: %s%, type %s%") % object_name % object_type).str());
        }
      }
    }
  }
  CATCH_AND_TRANSLATE();
}

std::vector<std::string> Schema::get_members() const
{
  std::vector<std::string> members(DatabaseObject::get_members());
  members.push_back("tables");
  members.push_back("collections");
  members.push_back("views");

  for (Value::Map_type::const_iterator iter = _tables->begin();
       iter != _tables->end(); ++iter)
  {
    members.push_back(iter->first);
  }
  return members;
}

Value Schema::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is must be either a table, collection or view and attempt loading it as such
  if (DatabaseObject::has_member(prop))
    ret_val = DatabaseObject::get_member(prop);
  else if (prop == "tables")
    ret_val = Value(_tables);
  else if (prop == "collections")
    ret_val = Value(_collections);
  else if (prop == "views")
    ret_val = Value(_views);
  else
  {
    // At this point the property should be one of table
    // collection or view
    ret_val = find_in_collection(prop, _tables);

    if (!ret_val)
      ret_val = find_in_collection(prop, _collections);

    if (!ret_val)
      ret_val = find_in_collection(prop, _views);

    if (!ret_val)
      ret_val = _load_object(prop);

    if (!ret_val)
      throw Exception::attrib_error("Invalid object member " + prop);
  }

  return ret_val;
}

shcore::Value Schema::find_in_collection(const std::string& name, boost::shared_ptr<shcore::Value::Map_type>source) const
{
  Value::Map_type::const_iterator iter = source->find(name);
  if (iter != source->end())
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();
}

Value Schema::_load_object(const std::string& name, const std::string& type) const
{
  Value ret_val;
  try
  {
    boost::shared_ptr<ApiBaseSession> sess(boost::dynamic_pointer_cast<ApiBaseSession>(_session.lock()));
    if (sess)
    {
      {
        sess->flush_last_result();

        std::auto_ptr< ::mysqlx::Result> result(sess->session_obj()->executeSql("show full tables in `" + _name + "` like '" + name + "';"));
        std::auto_ptr< ::mysqlx::Row> row(result->next());
        if (row.get())
        {
          std::string object_name = row->stringField(0);
          std::string object_type = row->stringField(1);

          if (type.empty() || (type == "TABLE" && (object_type == "BASE TABLE" || object_type == "LOCAL TEMPORARY")))
          {
            boost::shared_ptr<Table> table(new Table(shared_from_this(), object_name));
            ret_val = Value(boost::static_pointer_cast<Object_bridge>(table));
            (*_tables)[name] = ret_val;
          }
          else if (type.empty() || (type == "VIEW" && (object_type == "VIEW" || object_type == "SYSTEM VIEW")))
          {
            boost::shared_ptr<View> view(new View(shared_from_this(), object_name));
            ret_val = Value(boost::static_pointer_cast<Object_bridge>(view));
            (*_views)[name] = ret_val;
          }
          else if (type.empty() || (type == "COLLECTION"))
          {
            boost::shared_ptr<Collection> collection(new Collection(shared_from_this(), object_name));
            ret_val = Value(boost::static_pointer_cast<Object_bridge>(collection));
            (*_collections)[name] = ret_val;
          }
        }

        result->discardData();
      }
    }
  }
  CATCH_AND_TRANSLATE();

  return ret_val;
}

shcore::Value Schema::getTable(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + "::getTable").c_str());

  std::string name = args.string_at(0);

  return find_in_collection(name, _tables);
}

shcore::Value Schema::getCollection(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + "::getCollection").c_str());

  std::string name = args.string_at(0);

  return find_in_collection(name, _collections);
}

shcore::Value Schema::getView(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + "::getCollection").c_str());

  std::string name = args.string_at(0);

  return find_in_collection(name, _views);
}

shcore::Value Schema::createCollection(const shcore::Argument_list &args)
{
  Value ret_val;

  args.ensure_count(1, (class_name() + "::createCollection").c_str());

  // Creates the collection on the server
  shcore::Argument_list command_args;
  command_args.push_back(Value(_name));
  command_args.push_back(args[0]);

  boost::shared_ptr<ApiBaseSession> sess(boost::static_pointer_cast<ApiBaseSession>(_session.lock()));
  sess->executeAdminCommand("create_collection", command_args);

  // If this is reached it imlies all went OK on the previous operation
  std::string name = args.string_at(0);
  boost::shared_ptr<Collection> collection(new Collection(shared_from_this(), name));
  ret_val = Value(boost::static_pointer_cast<Object_bridge>(collection));
  (*_collections)[name] = ret_val;

  return ret_val;
}