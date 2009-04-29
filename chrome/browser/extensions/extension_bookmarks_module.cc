// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_bookmarks_module.h"

#include "base/json_writer.h"
#include "chrome/browser/bookmarks/bookmark_codec.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/extensions/extension_message_service.h"
#include "chrome/browser/profile.h"

namespace {
// keys
const wchar_t* kIdKey = L"id";
const wchar_t* kIndexKey = L"index";
const wchar_t* kParentIdKey = L"parentId";
const wchar_t* kOldIndexKey = L"oldIndex";
const wchar_t* kOldParentIdKey = L"oldParentId";
const wchar_t* kUrlKey = L"url";
const wchar_t* kTitleKey = L"title";
const wchar_t* kChildrenIdsKey = L"childrenIds";
const wchar_t* kChildrenKey = L"childrenIds";
const wchar_t* kRecursiveKey = L"recursive";

// errors
const char* kNoNodeError = "Can't find bookmark for id.";
const char* kNoParentError = "Can't find parent bookmark for id.";
const char* kFolderNotEmptyError =
    "Can't remove non-empty folder (use recursive to force).";
const char* kInvalidIndexError = "Index out of bounds.";
const char* kInvalidUrlError = "Invalid URL.";
const char* kModifySpecialError = "Can't modify the root bookmark folders.";

// events
const char* kOnBookmarkAdded = "bookmark-added";
const char* kOnBookmarkRemoved = "bookmark-removed";
const char* kOnBookmarkChanged = "bookmark-changed";
const char* kOnBookmarkMoved = "bookmark-moved";
const char* kOnBookmarkChildrenReordered = "bookmark-children-reordered";
};

// Helper functions.
class ExtensionBookmarks {
 public:
  // Convert |node| into a JSON value
  static DictionaryValue* GetNodeDictionary(BookmarkNode* node, bool recurse) {
    DictionaryValue* dict = new DictionaryValue();
    dict->SetInteger(kIdKey, node->id());

    BookmarkNode* parent = node->GetParent();
    if (parent)
      dict->SetInteger(kParentIdKey, parent->id());

    if (!node->is_folder())
      dict->SetString(kUrlKey, node->GetURL().spec());

    dict->SetString(kTitleKey, node->GetTitle());

    int childCount = node->GetChildCount();
    ListValue* children = new ListValue();
    for (int i = 0; i < childCount; ++i) {
      BookmarkNode* child = node->GetChild(i);
      if (recurse) {
        DictionaryValue* dict = GetNodeDictionary(child, true);
        children->Append(dict);
      } else {
        Value* child_id = new FundamentalValue(child->id());
        children->Append(child_id);
      }
    }
    if (recurse)
      dict->Set(kChildrenKey, children);
    else
      dict->Set(kChildrenIdsKey, children);
    return dict;
  }

  // Add a JSON representation of |node| to the JSON |list|.
  static void AddNode(BookmarkNode* node, ListValue* list, bool recurse) {
    DictionaryValue* dict = GetNodeDictionary(node, recurse);
    list->Append(dict);
  }

 private:
  ExtensionBookmarks();
};

void BookmarksFunction::Run() {
  // TODO(erikkay) temporary hack until adding an event listener can notify the
  // browser.
  ExtensionBookmarkEventRouter* event_router =
      ExtensionBookmarkEventRouter::GetSingleton();
  BookmarkModel* model = profile()->GetBookmarkModel();
  event_router->Observe(model);
  SyncExtensionFunction::Run();
}

// static
ExtensionBookmarkEventRouter* ExtensionBookmarkEventRouter::GetSingleton() {
  return Singleton<ExtensionBookmarkEventRouter>::get();
}

ExtensionBookmarkEventRouter::ExtensionBookmarkEventRouter() {
}

ExtensionBookmarkEventRouter::~ExtensionBookmarkEventRouter() {
}

void ExtensionBookmarkEventRouter::Observe(BookmarkModel* model) {
  if (models_.find(model) == models_.end()) {
    model->AddObserver(this);
    models_.insert(model);
  }
}

void ExtensionBookmarkEventRouter::DispatchEvent(Profile *profile,
                                                 const char* event_name,
                                                 const std::string json_args) {
  ExtensionMessageService::GetInstance(profile->GetRequestContext())->
      DispatchEventToRenderers(event_name, json_args);
}

void ExtensionBookmarkEventRouter::Loaded(BookmarkModel* model) {
  // TODO(erikkay): Do we need an event here?  It seems unlikely that
  // an extension would load before bookmarks loaded.
}

void ExtensionBookmarkEventRouter::BookmarkNodeMoved(BookmarkModel* model,
                                                     BookmarkNode* old_parent,
                                                     int old_index,
                                                     BookmarkNode* new_parent,
                                                     int new_index) {
  ListValue args;
  DictionaryValue* object_args = new DictionaryValue();
  BookmarkNode* node = new_parent->GetChild(new_index);
  object_args->SetInteger(kIdKey, node->id());
  object_args->SetInteger(kParentIdKey, new_parent->id());
  object_args->SetInteger(kIndexKey, new_index);
  object_args->SetInteger(kOldParentIdKey, old_parent->id());
  object_args->SetInteger(kOldIndexKey, old_index);
  args.Append(object_args);

  std::string json_args;
  JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), kOnBookmarkMoved, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeAdded(BookmarkModel* model,
                                                     BookmarkNode* parent,
                                                     int index) {
  ListValue args;
  DictionaryValue* object_args = new DictionaryValue();
  BookmarkNode* node = parent->GetChild(index);
  object_args->SetInteger(kIdKey, node->id());
  object_args->SetString(kTitleKey, node->GetTitle());
  object_args->SetString(kUrlKey, node->GetURL().spec());
  object_args->SetInteger(kParentIdKey, parent->id());
  object_args->SetInteger(kIndexKey, index);
  args.Append(object_args);

  std::string json_args;
  JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), kOnBookmarkAdded, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeRemoved(BookmarkModel* model,
                                                       BookmarkNode* parent,
                                                       int index) {
  ListValue args;
  DictionaryValue* object_args = new DictionaryValue();
  object_args->SetInteger(kParentIdKey, parent->id());
  object_args->SetInteger(kIndexKey, index);
  args.Append(object_args);

  std::string json_args;
  JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), kOnBookmarkRemoved, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeChanged(BookmarkModel* model,
                                                       BookmarkNode* node) {
  ListValue args;
  args.Append(new FundamentalValue(node->id()));

  // TODO(erikkay) The only two things that BookmarkModel sends this
  // notification for are title and favicon.  Since we're currently ignoring
  // favicon and since the notification doesn't say which one anyway, for now
  // we only include title.  The ideal thing would be to change BookmarkModel
  // to indicate what changed.
  DictionaryValue* object_args = new DictionaryValue();
  object_args->SetString(kTitleKey, node->GetTitle());
  args.Append(object_args);

  std::string json_args;
  JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), kOnBookmarkChanged, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeFavIconLoaded(
    BookmarkModel* model, BookmarkNode* node) {
  // TODO(erikkay) anything we should do here?
}

void ExtensionBookmarkEventRouter::BookmarkNodeChildrenReordered(
    BookmarkModel* model, BookmarkNode* node) {
  ListValue args;
  args.Append(new FundamentalValue(node->id()));
  int childCount = node->GetChildCount();
  ListValue* children = new ListValue();
  for (int i = 0; i < childCount; ++i) {
    BookmarkNode* child = node->GetChild(i);
    Value* child_id = new FundamentalValue(child->id());
    children->Append(child_id);
  }
  args.Append(children);

  std::string json_args;
  JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), kOnBookmarkChildrenReordered, json_args);
}

bool GetBookmarksFunction::RunImpl() {
  // TODO(erikkay): the JSON schema doesn't support the TYPE_INTEGER
  // variant yet.
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST) ||
                              args_->IsType(Value::TYPE_INTEGER) ||
                              args_->IsType(Value::TYPE_NULL));
  BookmarkModel* model = profile()->GetBookmarkModel();
  scoped_ptr<ListValue> json(new ListValue());
  if (args_->IsType(Value::TYPE_INTEGER)) {
    int id;
    EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&id));
    BookmarkNode* node = model->GetNodeByID(id);
    if (!node) {
      error_ = kNoNodeError;
      return false;
    }
    ExtensionBookmarks::AddNode(node, json.get(), false);
  } else {
    ListValue* ids = NULL;
    size_t count = 0;
    if (args_->IsType(Value::TYPE_LIST)) {
      ids = static_cast<ListValue*>(args_);
      count = ids->GetSize();
    }
    if (count == 0) {
      // If no ids are passed in, then we default to returning the root node.
      BookmarkNode* node = model->root_node();
      ExtensionBookmarks::AddNode(node, json.get(), false);
    } else {
      for (size_t i = 0; i < count; ++i) {
        int id = 0;
        EXTENSION_FUNCTION_VALIDATE(ids->GetInteger(i, &id));
        BookmarkNode* node = model->GetNodeByID(id);
        if (!node) {
          error_ = kNoNodeError;
          return false;
        } else {
          ExtensionBookmarks::AddNode(node, json.get(), false);
        }
      }
      if (error_.size() && json->GetSize() == 0) {
        return false;
      }
    }
  }

  result_.reset(json.release());
  return true;
}

bool GetBookmarkChildrenFunction::RunImpl() {
  int id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&id));
  BookmarkModel* model = profile()->GetBookmarkModel();
  scoped_ptr<ListValue> json(new ListValue());
  BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = kNoNodeError;
    return false;
  }
  int child_count = node->GetChildCount();
  for (int i = 0; i < child_count; ++i) {
    BookmarkNode* child = node->GetChild(i);
    ExtensionBookmarks::AddNode(child, json.get(), false);
  }

  result_.reset(json.release());
  return true;
}

bool GetBookmarkTreeFunction::RunImpl() {
  BookmarkModel* model = profile()->GetBookmarkModel();
  scoped_ptr<ListValue> json(new ListValue());
  BookmarkNode* node = model->root_node();
  ExtensionBookmarks::AddNode(node, json.get(), true);
  result_.reset(json.release());
  return true;
}

bool SearchBookmarksFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_STRING));

  std::wstring query;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsString(&query));

  BookmarkModel* model = profile()->GetBookmarkModel();
  ListValue* json = new ListValue();
  std::vector<BookmarkNode*> nodes;
  bookmark_utils::GetBookmarksContainingText(model, query, 50, &nodes);
  std::vector<BookmarkNode*>::iterator i = nodes.begin();
  for (; i != nodes.end(); ++i) {
    BookmarkNode* node = *i;
    ExtensionBookmarks::AddNode(node, json, false);
  }

  result_.reset(json);
  return true;
}

bool RemoveBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  DictionaryValue* json = static_cast<DictionaryValue*>(args_);

  // TODO(erikkay): it would be cool to take a list here as well.
  int id;
  EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kIdKey, &id));

  bool recursive = false;
  json->GetBoolean(kRecursiveKey, &recursive);  // optional

  BookmarkModel* model = profile()->GetBookmarkModel();
  BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = kNoNodeError;
    return false;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    error_ = kModifySpecialError;
    return false;
  }
  if (node->is_folder() && node->GetChildCount() > 0 && !recursive) {
    error_ = kFolderNotEmptyError;
    return false;
  }

  BookmarkNode* parent = node->GetParent();
  if (!parent) {
    error_ = kNoParentError;
    return false;
  }
  int index = parent->IndexOfChild(node);
  model->Remove(parent, index);
  return true;
}

bool CreateBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  DictionaryValue* json = static_cast<DictionaryValue*>(args_);

  BookmarkModel* model = profile()->GetBookmarkModel();
  int parentId;
  if (!json->HasKey(kParentIdKey)) {  // optional, default to "other bookmarks"
    parentId = model->other_node()->id();
  } else {
    EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kParentIdKey, &parentId));
  }
  BookmarkNode* parent = model->GetNodeByID(parentId);
  if (!parent) {
    error_ = kNoParentError;
    return false;
  }
  if (parent->GetParent() == NULL) {  // can't create children of the root
    error_ = kNoParentError;
    return false;
  }

  int index;
  if (!json->HasKey(kIndexKey)) {  // optional (defaults to end)
    index = parent->GetChildCount();
  } else {
    EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kIndexKey, &index));
    if (index > parent->GetChildCount() || index < 0) {
      error_ = kInvalidIndexError;
      return false;
    }
  }

  std::wstring title;
  json->GetString(kTitleKey, &title);  // optional
  std::string url_string;
  json->GetString(kUrlKey, &url_string);  // optional
  GURL url(url_string);
  if (!url.is_empty() && !url.is_valid()) {
    error_ = kInvalidUrlError;
    return false;
  }

  BookmarkNode* node;
  if (url_string.length())
    node = model->AddURL(parent, index, title, url);
  else
    node = model->AddGroup(parent, index, title);
  DCHECK(node);
  if (!node) {
    error_ = kNoNodeError;
    return false;
  }

  DictionaryValue* ret = ExtensionBookmarks::GetNodeDictionary(node, false);
  result_.reset(ret);

  return true;
}

bool MoveBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  DictionaryValue* json = static_cast<DictionaryValue*>(args_);

  // TODO(erikkay) it would be cool if this could be a list of ids as well
  int id = 0;
  EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kIdKey, &id));

  BookmarkModel* model = profile()->GetBookmarkModel();
  BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = kNoNodeError;
    return false;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    error_ = kModifySpecialError;
    return false;
  }

  BookmarkNode* parent;
  if (!json->HasKey(kParentIdKey)) {  // optional, defaults to current parent
    parent = node->GetParent();
  } else {
    int parentId;
    EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kParentIdKey, &parentId));
    parent = model->GetNodeByID(parentId);
  }
  if (!parent) {
    error_ = kNoParentError;
    // TODO(erikkay) return an error message
    return false;
  }
  if (parent == model->root_node()) {
    error_ = kModifySpecialError;
    return false;
  }

  int index;
  if (json->HasKey(kIndexKey)) {  // optional (defaults to end)
    EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kIndexKey, &index));
    if (index > parent->GetChildCount() || index < 0) {
      error_ = kInvalidIndexError;
      return false;
    }
  } else {
    index = parent->GetChildCount();
  }

  model->Move(node, parent, index);
  return true;
}

bool SetBookmarkTitleFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  DictionaryValue* json = static_cast<DictionaryValue*>(args_);

  std::wstring title;
  json->GetString(kTitleKey, &title);  // optional (empty is clear)

  BookmarkModel* model = profile()->GetBookmarkModel();
  int id = 0;
  EXTENSION_FUNCTION_VALIDATE(json->GetInteger(kIdKey, &id));
  BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = kNoNodeError;
    return false;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    error_ = kModifySpecialError;
    return false;
  }
  model->SetTitle(node, title);
  return true;
}
