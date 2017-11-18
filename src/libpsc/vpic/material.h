
#ifndef MATERIAL_H
#define MATERIAL_H

// ======================================================================
// MaterialList

struct MaterialList {
  MaterialList(material_t*& m);

  material_t* append(material_t* m);
  bool empty();
  
  //private:
  material_t *&ml_;
};

inline MaterialList::MaterialList(material_t*& m)
  : ml_(m)
{
}

inline material_t* MaterialList::append(material_t* m)
{
  return ::append_material(m, &ml_);
}

inline bool MaterialList::empty()
{
  return !ml_;
}


#endif