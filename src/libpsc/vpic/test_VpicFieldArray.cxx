
#include "test_FieldArray.h"

#include "VpicFieldArray.h"
#include "VpicFieldArrayLocalOps.h"
#include "VpicFieldArrayOps.h"

void test_VpicFieldArray()
{
  typedef VpicFieldArray<VpicFieldArrayBase> FieldArray;

  Grid grid;
  MaterialList material_list;
  FieldArray fa(&grid, material_list, 0.);

  test_FieldArray_methods(fa);
}

int main(int argc, char **argv)
{
  test_VpicFieldArray();
}