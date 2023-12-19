<abi-instr version='1.0'>
  <type-decl name='int' size-in-bits='32' alignment-in-bits='32' id='type-id-1'/>
  <type-decl name='char' size-in-bits='8' alignment-in-bits='8' id='type-id-2'/>
  <type-decl name='unsigned int' size-in-bits='32' alignment-in-bits='32' id='type-id-3'/>
  <class-decl name='s0' visibility='default' is-declaration-only='yes' id='type-id-4'/>
  <pointer-type-def type-id='type-id-4' size-in-bits='64' alignment-in-bits='64' id='type-id-5'/>
  <type-decl name='void' alignment-in-bits='8' id='type-id-6'/>
  <qualified-type-def type-id='type-id-4' const='yes' filepath='../../abitests/test-struct1-v0.cc' line='1' column='8' id='type-id-7'/>
  <pointer-type-def type-id='type-id-7' size-in-bits='64' alignment-in-bits='64' id='type-id-8'/>
  <class-decl name='s0' size-in-bits='192' alignment-in-bits='64' visibility='default' filepath='../../abitests/test-struct1-v0.cc' line='1' column='8' def-of-decl-id='type-id-4' id='type-id-9'>
    <member-type access='public'>
      <typedef-decl name='s0' type-id='type-id-4' filepath='../../abitests/test-struct1-v0.cc' line='2' column='1' id='type-id-10'/>
    </member-type>
    <member-type access='public'>
      <typedef-decl name='number_type' type-id='type-id-1' filepath='../../abitests/test-struct1-v0.cc' line='7' column='15' id='type-id-11'/>
    </member-type>
    <data-member access='public' layout-offset-in-bits='64'>
      <var-decl name='m0' type-id='type-id-1' visibility='default' filepath='../../abitests/test-struct1-v0.cc' line='3' column='7'/>
    </data-member>
    <data-member access='public' layout-offset-in-bits='96'>
      <var-decl name='m1' type-id='type-id-2' visibility='default' filepath='../../abitests/test-struct1-v0.cc' line='4' column='8'/>
    </data-member>
    <data-member access='public' layout-offset-in-bits='128'>
      <var-decl name='m2' type-id='type-id-3' visibility='default' filepath='../../abitests/test-struct1-v0.cc' line='5' column='12'/>
    </data-member>
    <member-function access='public' constructor='yes'>
      <function-decl name='__base_ctor ' mangled-name='_ZN2s0C2Ev' filepath='../../abitests/test-struct1-v0.cc' line='16' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-5'/>
        <return type-id='type-id-6'/>
      </function-decl>
    </member-function>
    <member-function access='public' constructor='yes'>
      <function-decl name='__comp_ctor ' mangled-name='_ZN2s0C1Ev' filepath='../../abitests/test-struct1-v0.cc' line='16' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-5'/>
        <return type-id='type-id-6'/>
      </function-decl>
    </member-function>
    <member-function access='public' destructor='yes'>
      <function-decl name='__base_dtor ' mangled-name='_ZN2s0D2Ev' filepath='../../abitests/test-struct1-v0.cc' line='20' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-5'/>
        <return type-id='type-id-6'/>
      </function-decl>
    </member-function>
    <member-function access='public' destructor='yes'>
      <function-decl name='__comp_dtor ' mangled-name='_ZN2s0D1Ev' filepath='../../abitests/test-struct1-v0.cc' line='20' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-5'/>
        <return type-id='type-id-6'/>
      </function-decl>
    </member-function>
    <member-function access='public' destructor='yes' vtable-offset='1'>
      <function-decl name='__deleting_dtor ' mangled-name='_ZN2s0D0Ev' filepath='../../abitests/test-struct1-v0.cc' line='20' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-5'/>
        <return type-id='type-id-6'/>
      </function-decl>
    </member-function>
    <member-function access='public' const='yes'>
      <function-decl name='foo' mangled-name='_ZNK2s03fooEv' filepath='../../abitests/test-struct1-v0.cc' line='25' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
        <parameter type-id='type-id-8'/>
      </function-decl>
    </member-function>
  </class-decl>
  <reference-type-def kind='lvalue' type-id='type-id-9' size-in-bits='64' alignment-in-bits='64' id='type-id-12'/>
  <function-decl name='bar' mangled-name='_Z3barR2s0' filepath='../../abitests/test-struct1-v0.cc' line='29' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
    <parameter type-id='type-id-12'/>
    <return type-id='type-id-1'/>
  </function-decl>
  <class-decl name='__class_type_info_pseudo' size-in-bits='128' alignment-in-bits='64' visibility='default' id='type-id-13'/>
  <qualified-type-def type-id='type-id-13' const='yes' id='type-id-14'/>
  <var-decl name='_ZTI2s0' type-id='type-id-14' mangled-name='_ZTI2s0' visibility='default' binding='weak' filepath='../../abitests/test-struct1-v0.cc' line='1' column='8'/>
</abi-instr>
