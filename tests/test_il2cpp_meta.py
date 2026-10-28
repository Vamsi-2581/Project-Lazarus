import importlib.util
import struct
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "il2cpp_meta.py"
SPEC = importlib.util.spec_from_file_location("il2cpp_meta", SCRIPT)
il2cpp_meta = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(il2cpp_meta)


def synthetic_metadata():
    strings = bytearray(b"\0")
    indexes = {}
    for value in ("Game", "Outer", "Inner", "Leaf"):
        indexes[value] = len(strings)
        strings.extend(value.encode("utf-8") + b"\0")

    def type_definition(name, declaring_type_index):
        fields = [0] * 26
        fields[0] = indexes[name]
        fields[1] = indexes["Game"]
        fields[3] = declaring_type_index
        return il2cpp_meta.TYPE_DEF.pack(*fields)

    definitions = b"".join(
        (
            type_definition("Outer", -1),
            type_definition("Inner", 0),
            type_definition("Leaf", 1),
        )
    )
    header_size = 8 + len(il2cpp_meta.HEADER_SECTIONS) * 8
    type_offset = header_size
    string_offset = type_offset + len(definitions)
    data = bytearray(string_offset + len(strings))
    struct.pack_into("<Ii", data, 0, il2cpp_meta.SANITY, 29)
    type_section = il2cpp_meta.HEADER_SECTIONS.index("typeDefinitions")
    string_section = il2cpp_meta.HEADER_SECTIONS.index("string")
    struct.pack_into("<ii", data, 8 + type_section * 8, type_offset, len(definitions))
    struct.pack_into("<ii", data, 8 + string_section * 8, string_offset, len(strings))
    data[type_offset:string_offset] = definitions
    data[string_offset:] = strings
    return il2cpp_meta.Metadata(bytes(data)).load()


class NestedTypeNameTests(unittest.TestCase):
    def test_top_level_name_keeps_namespace(self):
        self.assertEqual(synthetic_metadata().type_name(0), "Game.Outer")

    def test_nested_names_include_each_declaring_type(self):
        metadata = synthetic_metadata()
        self.assertEqual(metadata.type_name(1), "Game.Outer/Inner")
        self.assertEqual(metadata.type_name(2), "Game.Outer/Inner/Leaf")


if __name__ == "__main__":
    unittest.main()
