#include "World/Gen.hpp"

void GenDesc::add_pass(std::shared_ptr<GenPass> pass)
{
    pass->init(*this);
    m_passes.push_back(pass);
}

void GenDesc::add_struct_pass(std::shared_ptr<StructurePass> pass)
{
    pass->init(*this);
    m_struct_passes.push_back(pass);
}
