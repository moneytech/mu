//: Exclusive containers contain exactly one of a fixed number of 'variants'
//: of different types.
//:
//: They also implicitly contain a tag describing precisely which variant is
//: currently stored in them.

:(before "End Mu Types Initialization")
//: We'll use this container as a running example, with two number elements.
{
type_ordinal tmp = Type_ordinal["number-or-point"] = Next_type_ordinal++;
Type[tmp].size = 2;
Type[tmp].kind = exclusive_container;
Type[tmp].name = "number-or-point";
vector<type_ordinal> t1;
t1.push_back(number);
Type[tmp].elements.push_back(t1);
vector<type_ordinal> t2;
t2.push_back(point);
Type[tmp].elements.push_back(t2);
Type[tmp].element_names.push_back("i");
Type[tmp].element_names.push_back("p");
}

//: Tests in this layer often explicitly setup memory before reading it as an
//: array. Don't do this in general. I'm tagging exceptions with /raw to
//: avoid errors.
:(scenario copy_exclusive_container)
# Copying exclusive containers copies all their contents and an extra location for the tag.
recipe main [
  1:number <- copy 1  # 'point' variant
  2:number <- copy 34
  3:number <- copy 35
  4:number-or-point <- copy 1:number-or-point/raw  # unsafe
]
+mem: storing 1 in location 4
+mem: storing 34 in location 5
+mem: storing 35 in location 6

:(before "End size_of(types) Cases")
if (t.kind == exclusive_container) {
  // size of an exclusive container is the size of its largest variant
  // (So like containers, it can't contain arrays.)
  long long int result = 0;
  for (long long int i = 0; i < t.size; ++i) {
    long long int tmp = size_of(t.elements.at(i));
    if (tmp > result) result = tmp;
  }
  // ...+1 for its tag.
  return result+1;
}

//:: To access variants of an exclusive container, use 'maybe-convert'.
//: It always returns an address (so that you can modify it) or null (to
//: signal that the conversion failed (because the container contains a
//: different variant).

//: 'maybe-convert' requires a literal in ingredient 1. We'll use a synonym
//: called 'variant'.
:(before "End Mu Types Initialization")
Type_ordinal["variant"] = 0;

:(scenario maybe_convert)
recipe main [
  12:number <- copy 1
  13:number <- copy 35
  14:number <- copy 36
  20:address:point <- maybe-convert 12:number-or-point/raw, 1:variant  # unsafe
]
+mem: storing 13 in location 20

:(scenario maybe_convert_fail)
recipe main [
  12:number <- copy 1
  13:number <- copy 35
  14:number <- copy 36
  20:address:point <- maybe-convert 12:number-or-point/raw, 0:variant  # unsafe
]
+mem: storing 0 in location 20

:(before "End Primitive Recipe Declarations")
MAYBE_CONVERT,
:(before "End Primitive Recipe Numbers")
Recipe_ordinal["maybe-convert"] = MAYBE_CONVERT;
:(before "End Primitive Recipe Checks")
case MAYBE_CONVERT: {
  if (SIZE(inst.ingredients) != 2) {
    raise_error << maybe(Recipe[r].name) << "'maybe-convert' expects exactly 2 ingredients in '" << inst.to_string() << "'\n" << end();
    break;
  }
  reagent base = inst.ingredients.at(0);
  canonize_type(base);
  if (base.types.empty() || Type[base.types.at(0)].kind != exclusive_container) {
    raise_error << current_recipe_name () << "first ingredient of 'maybe-convert' should be an exclusive-container, but got " << base.original_string << '\n' << end();
    break;
  }
  if (!is_literal(inst.ingredients.at(1))) {
    raise_error << maybe(Recipe[r].name) << "second ingredient of 'maybe-convert' should have type 'variant', but got " << inst.ingredients.at(1).original_string << '\n' << end();
    break;
  }
  break;
}
:(before "End Primitive Recipe Implementations")
case MAYBE_CONVERT: {
  reagent base = canonize(current_instruction().ingredients.at(0));
  long long int base_address = base.value;
  if (base_address == 0) {
    raise_error << maybe(current_recipe_name()) << "tried to access location 0 in '" << current_instruction().to_string() << "'\n" << end();
    break;
  }
  long long int tag = current_instruction().ingredients.at(1).value;
  long long int result;
  if (tag == static_cast<long long int>(Memory[base_address])) {
    result = base_address+1;
  }
  else {
    result = 0;
  }
  products.resize(1);
  products.at(0).push_back(result);
  break;
}

//:: Allow exclusive containers to be defined in mu code.

:(scenario exclusive_container)
exclusive-container foo [
  x:number
  y:number
]
+parse: reading exclusive-container foo
+parse:   element name: x
+parse:   type: 1
+parse:   element name: y
+parse:   type: 1

:(before "End Command Handlers")
else if (command == "exclusive-container") {
  insert_container(command, exclusive_container, in);
}

//:: To construct exclusive containers out of variant types, use 'merge'.
:(scenario lift_to_exclusive_container)
exclusive-container foo [
  x:number
  y:number
]

recipe main [
  1:number <- copy 34
  2:foo <- merge 0/x, 1:number
  4:foo <- merge 1/x, 1:number
]
+mem: storing 0 in location 2
+mem: storing 34 in location 3
+mem: storing 1 in location 4
+mem: storing 34 in location 5

//: Since the different variants of an exclusive-container might have
//: different sizes, relax the size mismatch check for 'merge' instructions.
:(before "End size_mismatch(x) Cases")
if (current_instruction().operation == MERGE
    && !current_instruction().products.empty()
    && !current_instruction().products.at(0).types.empty()) {
  reagent x = canonize(current_instruction().products.at(0));
  if (Type[x.types.at(0)].kind == exclusive_container) {
    return size_of(x) < SIZE(data);
  }
}

:(scenario merge_exclusive_container_with_mismatched_sizes)
container foo [
  x:number
  y:number
]

exclusive-container bar [
  x:number
  y:foo
]

recipe main [
  1:number <- copy 34
  2:number <- copy 35
  3:bar <- merge 0/x, 1:number
  6:bar <- merge 1/foo, 1:number, 2:number
]
+mem: storing 0 in location 3
+mem: storing 34 in location 4
# bar is always 3 large so location 5 is skipped
+mem: storing 1 in location 6
+mem: storing 34 in location 7
+mem: storing 35 in location 8
