import pythonmonkey as pm

pm.eval("""
class Person {
    constructor(firstName, lastName, age) {
        this.firstName = firstName;
        this.lastName = lastName;
        this.age = age;
    }

    getFullName() {
        return `${this.firstName} ${this.lastName}`;
    }

    getAge() {
        return this.age;
    }
}
""")

Person = pm.eval("Person")


def test_new_instanceof_js_constructor_function_passed_to_py():
  will_pringle = pm.new(Person)('Will', 'Pringle', 23);
  assert(pm.eval('(p) => p instanceof Person')(will_pringle))

def test_new_instanceof_string_constructor_in_js():
  jane_doe = pm.new('Person')('Jane', 'Doe', 99)
  assert(pm.eval('(p) => p instanceof Person')(jane_doe))

def test_new_property_initialization():
  pass

def test_new_variable_parameters():
  pass

def test_new_methods():
  pass

def test_new_static_methods():
  pass

def test_new_object_indepdendence():
  pass

def test_new_empty():
  pass

def test_new_nonexistant_constructor():
  pass

def test_new_class_with_no_constructor():
  pass

def test_new_null_prototype():
  pass

def test_new_inherited_classes():
  pass

def test_new_constructor_return():
  pass

def test_new_binding_this_persistence():
  """
class SelfAwarePerson {
    constructor() {
        this.isThisInstance = (this instanceof SelfAwarePerson);
    }
}

const createSelfAwarePerson = pm.new(SelfAwarePerson);
const person = createSelfAwarePerson();
console.assert(person.isThisInstance, '`this` is not correctly bound within the constructor');
  """
  pass

def test_new_WebAssembly_Module():
  pass

def test_new_Date():
  d1 = pm.new("Date", 0)
  d2 = pm.eval("(() => new Date(0))()")
  pass

