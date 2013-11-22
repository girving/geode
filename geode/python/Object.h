//#####################################################################
// Class Object
//#####################################################################
//
// Object is a base class for C++ objects which are also python objects.
//
// Since python objects are reference counted, instances of Object can be owned only through Ref<T> or Ptr<T>,
// which manage the reference count.  To allocate a new object of type T, use new_<T>(...), which returns a Ref.
//
// Classes derived from object are noncopyable by default.
//
// Note that the python header is stored immediately _before_ the start of the C++ object, since C++ insists on
// putting the vtable at the start of objects.  Thus a PyObject* p can be converted to a C++ T* via (T*)(p+1),
// and a T* q can be converted to PyObject* via (PyObject*)q-1.
//
// Since they are exposed to python directly by casting T* to PyObject*, it is impossible to preserve const
// semantics across the C++ python layer: any information about immutability must apply to the object itself,
// not each reference to the object.
//
// Descendants of Object must use GEODE_DECLARE_TYPE / GEODE_DEFINE_TYPE to expose information about the derived
// type to the python wrapping layer.
//
//#####################################################################
#pragma once

#include <geode/python/config.h>
#include <geode/python/forward.h>
#include <geode/python/new.h>
#include <geode/utility/config.h>
#include <boost/mpl/void.hpp>
#include <boost/utility/enable_if.hpp>

namespace geode {

namespace mpl = boost::mpl;

class GEODE_CORE_CLASS_EXPORT Object {
public:
  GEODE_DECLARE_TYPE(GEODE_CORE_EXPORT)
  typedef Object Base; // the hierarchy stops here
private:
  Object(const Object&); // noncopyable by default
  void operator=(const Object&); // noncopyable by default
protected:
  GEODE_CORE_EXPORT Object();
public:
  GEODE_CORE_EXPORT virtual ~Object();
};

// Helper for derived classes which are simple wrappers around other real classes
template<class T> struct GetSelf {
  typedef T type;
  static T* get(PyObject* object) {
    BOOST_MPL_ASSERT((boost::is_base_of<Object,T>));
    return (T*)(object+1);
  }
};

class WeakRefSupport;

template<class T, bool has_weakrefs = boost::is_base_of<WeakRefSupport,T>::value >
struct WeakRef_Helper {
  static int offset() { return 0; }
  static void clear_refs(PyObject *) {}
};

template<class T>
struct WeakRef_Helper<T, true> {
  static int offset() {
    // Enable weak reference support if the class derives from WeakRefSupport
    const T* test = (T*)0xff;
    PyObject * const * weaklist = &test->weakreflist;
    int offset = long(weaklist) - long(test) + sizeof(geode::PyObject);
    return offset;
  }

  static void clear_refs(PyObject *o) {
    T *t = GetSelf<T>::get(o);
    if (t->weakreflist != NULL) {
      PyObject_ClearWeakRefs(o);
      t->weakreflist = 0;
    }
  }
};

class GEODE_CORE_CLASS_EXPORT WeakRefSupport {
  template<class T, bool B> friend struct WeakRef_Helper;
private:
  PyObject *weakreflist;
protected:
  GEODE_CORE_EXPORT WeakRefSupport(): weakreflist(0) {}
};

struct ObjectUnusable {};

// Conversion from T& for python types
template<class T> static inline PyObject*
to_python(T& value, typename boost::enable_if<boost::is_base_of<Object,T>,ObjectUnusable>::type unusable=ObjectUnusable()) {
  PyObject* object = (PyObject*)&value-1;
  GEODE_INCREF(object);
  return object;
}

}
