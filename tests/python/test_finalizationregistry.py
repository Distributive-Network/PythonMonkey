import pythonmonkey as pm


def test_finalizationregistry():
  result = pm.eval("""
  (collect) => {
    let arr = [42, 43];
    const registry = new FinalizationRegistry(heldValue => { arr[heldValue] = heldValue; });
    let obj1 = {};
    let obj2 = {};
    registry.register(obj1, 0);
    registry.register(obj2, 1);
    obj1 = null;
    obj2 = null;

    collect();

    return arr;
  }
  """)(pm.collect)

  assert result[0] == 0
  assert result[1] == 1
