#include "axi.hpp"
#include "macro.hpp"
#include <set>

bool axi4_ptr::check() {/*{{{*/
    std::set <void*> s;
#define __S_INSERT__(width,name,input) s.insert((void*) name );
    AXI_BUNDLE(__S_INSERT__)
        return s.size() == 29 && s.count(NULL) == 0;
}/*}}}*/

axi4_ref::axi4_ref(axi4_ptr &ptr):/*{{{*/
#define __AXI_PTR_INIT_REF__(width,name,input) name (*(ptr.name))
    AXI_BUNDLE(__AXI_PTR_INIT_REF__, __COMMA__) {}/*}}}*/

axi4_ref::axi4_ref(axi4  &axi4):/*{{{*/
#define __AXI_VAR_INIT_REF__(width,name,input) name (axi4.name)
        AXI_BUNDLE(__AXI_VAR_INIT_REF__, __COMMA__) {}/*}}}*/

void axi4::update_input(axi4_ref &ref) {/*{{{*/
#define __AXI_VAR_IN__(width,name,input) IFZERO(input,name=ref.name;)
    AXI_BUNDLE(__AXI_VAR_IN__)
}/*}}}*/

void axi4::update_output(axi4_ref &ref) {/*{{{*/
#define __AXI_VAR_OUT__(width,name,input) IFONE(input,ref.name=name;)
    AXI_BUNDLE(__AXI_VAR_OUT__)
}/*}}}*/
