import pytest
import pythonmonkey as pm
import gc
import numpy, array, struct

def test_py_buffer_to_js_typed_array():
    # JS TypedArray/ArrayBuffer should coerce to Python memoryview type
    def assert_js_to_py_memoryview(buf: memoryview):
        assert type(buf) is memoryview
        assert None == buf.obj # https://docs.python.org/3.9/c-api/buffer.html#c.Py_buffer.obj
        assert 2 * 4 == buf.nbytes # 2 elements * sizeof(int32_t)
        assert "02000000ffffffff" == buf.hex() # native (little) endian
    buf1 = pm.eval("new Int32Array([2,-1])")
    buf2 = pm.eval("new Int32Array([2,-1]).buffer")
    assert_js_to_py_memoryview(buf1)
    assert_js_to_py_memoryview(buf2)
    assert [2, -1] == buf1.tolist()
    assert [2, 0, 0, 0, 255, 255, 255, 255] == buf2.tolist()
    assert -1 == buf1[1]
    assert 255 == buf2[7]
    with pytest.raises(IndexError, match="index out of bounds on dimension 1"):
        buf1[2]
    with pytest.raises(IndexError, match="index out of bounds on dimension 1"):
        buf2[8]
    del buf1, buf2

    # test element value ranges
    buf3 = pm.eval("new Uint8Array(1)")
    with pytest.raises(ValueError, match="memoryview: invalid value for format 'B'"):
        buf3[0] = 256
    with pytest.raises(ValueError, match="memoryview: invalid value for format 'B'"):
        buf3[0] = -1
    with pytest.raises(IndexError, match="index out of bounds on dimension 1"): # no automatic resize
        buf3[1] = 0
    del buf3

    # Python buffers should coerce to JS TypedArray
    # and the typecode maps to TypedArray subtype (Uint8Array, Float64Array, ...)
    assert True == pm.eval("(arr)=>arr instanceof Uint8Array")( bytearray([1,2,3]) )
    assert True == pm.eval("(arr)=>arr instanceof Uint8Array")( numpy.array([1], dtype=numpy.uint8) )
    assert True == pm.eval("(arr)=>arr instanceof Uint16Array")( numpy.array([1], dtype=numpy.uint16) )
    assert True == pm.eval("(arr)=>arr instanceof Uint32Array")( numpy.array([1], dtype=numpy.uint32) )
    assert True == pm.eval("(arr)=>arr instanceof BigUint64Array")( numpy.array([1], dtype=numpy.uint64) )
    assert True == pm.eval("(arr)=>arr instanceof Int8Array")( numpy.array([1], dtype=numpy.int8) )
    assert True == pm.eval("(arr)=>arr instanceof Int16Array")( numpy.array([1], dtype=numpy.int16) )
    assert True == pm.eval("(arr)=>arr instanceof Int32Array")( numpy.array([1], dtype=numpy.int32) )
    assert True == pm.eval("(arr)=>arr instanceof BigInt64Array")( numpy.array([1], dtype=numpy.int64) )
    assert True == pm.eval("(arr)=>arr instanceof Float32Array")( numpy.array([1], dtype=numpy.float32) )
    assert True == pm.eval("(arr)=>arr instanceof Float64Array")( numpy.array([1], dtype=numpy.float64) )
    assert pm.eval("new Uint8Array([1])").format == "B"
    assert pm.eval("new Uint16Array([1])").format == "H"
    assert pm.eval("new Uint32Array([1])").format == "I" # FIXME (Tom Tang): this is "L" on 32-bit systems
    assert pm.eval("new BigUint64Array([1n])").format == "Q"
    assert pm.eval("new Int8Array([1])").format == "b"
    assert pm.eval("new Int16Array([1])").format == "h"
    assert pm.eval("new Int32Array([1])").format == "i"
    assert pm.eval("new BigInt64Array([1n])").format == "q"
    assert pm.eval("new Float32Array([1])").format == "f"
    assert pm.eval("new Float64Array([1])").format == "d"

    # not enough bytes to populate an element of the TypedArray
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: buffer length for BigInt64Array should be a multiple of 8"):
        pm.eval("(arr) => new BigInt64Array(arr.buffer)")(array.array('i', [-11111111]))

    # TypedArray with `byteOffset` and `length`
    arr1 = array.array('i', [-11111111, 22222222, -33333333, 44444444])
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: invalid or out-of-range index"):
        pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ -4)")(arr1)
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: start offset of Int32Array should be a multiple of 4"):
        pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ 1)")(arr1)
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: size of buffer is too small for Int32Array with byteOffset"):
        pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ 20)")(arr1)
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: invalid or out-of-range index"):
        pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ 4, /*length*/ -1)")(arr1)
    with pytest.raises(pm.SpiderMonkeyError, match="RangeError: attempting to construct out-of-bounds Int32Array on ArrayBuffer"):
        pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ 4, /*length*/ 4)")(arr1)
    arr2 = pm.eval("(arr) => new Int32Array(arr.buffer, /*byteOffset*/ 4, /*length*/ 2)")(arr1)
    assert 2 * 4 == arr2.nbytes # 2 elements * sizeof(int32_t)
    assert [22222222, -33333333] == arr2.tolist()
    assert "8e155301ab5f03fe" == arr2.hex() # native (little) endian
    assert 22222222 == arr2[0] # offset 1 int32
    with pytest.raises(IndexError, match="index out of bounds on dimension 1"):
        arr2[2]
    arr3 = pm.eval("(arr) => new Int32Array(arr.buffer, 16 /* byteOffset */)")(arr1) # empty Int32Array
    assert 0 == arr3.nbytes
    del arr3

    # test GC
    del arr1
    gc.collect(), pm.collect()
    gc.collect(), pm.collect()
    # TODO (Tom Tang): the 0th element in the underlying buffer is still accessible after GC, even is not referenced by the JS TypedArray with byteOffset
    del arr2

    # mutation
    mut_arr_original = bytearray(4)
    pm.eval("""
    (/* @type Uint8Array */ arr) => {
        // 2.25 in float32 little endian
        arr[2] = 0x10
        arr[3] = 0x40
    }
    """)(mut_arr_original)
    assert 0x10 == mut_arr_original[2]
    assert 0x40 == mut_arr_original[3]
    # mutation to a different TypedArray accessing the same underlying data block will also change the original buffer
    def do_mutation(mut_arr_js):
        assert 2.25 == mut_arr_js[0]
        mut_arr_js[0] = 225.50048828125 # float32 little endian: 0x 20 80 61 43 
        assert "20806143" == mut_arr_original.hex()
        assert 225.50048828125 == array.array("f", mut_arr_original)[0]
    mut_arr_new = pm.eval("""
    (/* @type Uint8Array */ arr, do_mutation) => {
        const mut_arr_js = new Float32Array(arr.buffer)
        do_mutation(mut_arr_js)
        return arr
    }
    """)(mut_arr_original, do_mutation)
    assert [0x20, 0x80, 0x61, 0x43] == mut_arr_new.tolist()

    # simple 1-D numpy array should just work as well
    numpy_int16_array = numpy.array([0, 1, 2, 3], dtype=numpy.int16)
    assert "0,1,2,3" == pm.eval("(typedArray) => typedArray.toString()")(numpy_int16_array)
    assert 3.0 == pm.eval("(typedArray) => typedArray[3]")(numpy_int16_array)
    assert True == pm.eval("(typedArray) => typedArray instanceof Int16Array")(numpy_int16_array)
    numpy_memoryview = pm.eval("(typedArray) => typedArray")(numpy_int16_array)
    assert 2 == numpy_memoryview[2]
    assert 4 * 2 == numpy_memoryview.nbytes # 4 elements * sizeof(int16_t)
    assert "h" == numpy_memoryview.format # the type code for int16 is 'h', see https://docs.python.org/3.9/library/array.html
    with pytest.raises(IndexError, match="index out of bounds on dimension 1"):
        numpy_memoryview[4]

    # can work for empty Python buffer
    def assert_empty_py_buffer(buf, type: str):
        assert 0 == pm.eval("(typedArray) => typedArray.length")(buf)
        assert None == pm.eval("(typedArray) => typedArray[0]")(buf) # `undefined`
        assert True == pm.eval("(typedArray) => typedArray instanceof "+type)(buf)
    assert_empty_py_buffer(bytearray(b''), "Uint8Array")
    assert_empty_py_buffer(numpy.array([], dtype=numpy.uint64), "BigUint64Array")
    assert_empty_py_buffer(array.array('d', []), "Float64Array")

    # can work for empty TypedArray
    def assert_empty_typedarray(buf: memoryview, typecode: str):
        assert typecode == buf.format
        assert struct.calcsize(typecode) == buf.itemsize
        assert 0 == buf.nbytes
        assert "" == buf.hex()
        assert b"" == buf.tobytes()
        assert [] == buf.tolist()
        buf.release()
    assert_empty_typedarray(pm.eval("new BigInt64Array()"), "q")
    assert_empty_typedarray(pm.eval("new Float32Array(new ArrayBuffer(4), 4 /*byteOffset*/)"), "f")
    assert_empty_typedarray(pm.eval("(arr)=>arr")( bytearray([]) ), "B")
    assert_empty_typedarray(pm.eval("(arr)=>arr")( numpy.array([], dtype=numpy.uint16) ),"H")
    assert_empty_typedarray(pm.eval("(arr)=>arr")( array.array("d", []) ),"d")

    # can work for empty ArrayBuffer
    def assert_empty_arraybuffer(buf):
        assert "B" == buf.format
        assert 1 == buf.itemsize
        assert 0 == buf.nbytes
        assert "" == buf.hex()
        assert b"" == buf.tobytes()
        assert [] == buf.tolist()
        buf.release()
    assert_empty_arraybuffer(pm.eval("new ArrayBuffer()"))
    assert_empty_arraybuffer(pm.eval("new Uint8Array().buffer"))
    assert_empty_arraybuffer(pm.eval("new Float64Array().buffer"))
    assert_empty_arraybuffer(pm.eval("(arr)=>arr.buffer")( bytearray([]) ))
    assert_empty_arraybuffer(pm.eval("(arr)=>arr.buffer")( pm.eval("(arr)=>arr.buffer")(bytearray()) ))
    assert_empty_arraybuffer(pm.eval("(arr)=>arr.buffer")( numpy.array([], dtype=numpy.uint64) ))
    assert_empty_arraybuffer(pm.eval("(arr)=>arr.buffer")( array.array("d", []) ))

    # TODO (Tom Tang): shared ArrayBuffer should be disallowed
    # pm.eval("new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true }).buffer")

    # TODO (Tom Tang): once a JS ArrayBuffer is transferred to a worker thread, it should be invalidated in Python-land as well

    # TODO (Tom Tang): error for detached ArrayBuffer, or should it be considered as empty?

    # should error on immutable Python buffers 
    # Note: Python `bytes` type must be converted to a (mutable) `bytearray` because there's no such a concept of read-only ArrayBuffer in JS
    with pytest.raises(BufferError, match="Object is not writable."):
        pm.eval("(typedArray) => {}")(b'')
    immutable_numpy_array = numpy.arange(10)
    immutable_numpy_array.setflags(write=False)
    with pytest.raises(ValueError, match="buffer source array is read-only"):
        pm.eval("(typedArray) => {}")(immutable_numpy_array)

    # buffer should be in C order (row major)
    fortran_order_arr = numpy.array([[1, 2], [3, 4]], order="F") # 1-D array is always considered C-contiguous because it doesn't matter if it's row or column major in 1-D
    with pytest.raises(ValueError, match="ndarray is not C-contiguous"):
        pm.eval("(typedArray) => {}")(fortran_order_arr)

    # disallow multidimensional array
    numpy_2d_array = numpy.array([[1, 2], [3, 4]], order="C")
    with pytest.raises(BufferError, match="multidimensional arrays are not allowed"):
        pm.eval("(typedArray) => {}")(numpy_2d_array)
