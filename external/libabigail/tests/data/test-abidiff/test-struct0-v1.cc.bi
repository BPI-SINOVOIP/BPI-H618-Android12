<abi-instr version='1.0'>
  <type-decl name='int' size-in-bits='32' alignment-in-bits='32' id='type-id-1'/>
  <type-decl name='unsigned int' size-in-bits='32' alignment-in-bits='32' id='type-id-2'/>
  <type-decl name='char' size-in-bits='8' alignment-in-bits='8' id='type-id-3'/>
  <class-decl name='s0' visibility='default' is-declaration-only='yes' id='type-id-4'/>
  <class-decl name='s0' size-in-bits='96' alignment-in-bits='32' visibility='default' filepath='../../abitests/test-struct0-v1.cc' line='1' column='8' def-of-decl-id='type-id-4' id='type-id-5'>
    <member-type access='public'>
      <typedef-decl name='s0' type-id='type-id-4' filepath='../../abitests/test-struct0-v1.cc' line='2' column='1' id='type-id-6'/>
    </member-type>
    <data-member access='public' layout-offset-in-bits='0'>
      <var-decl name='m0' type-id='type-id-1' visibility='default' filepath='../../abitests/test-struct0-v1.cc' line='3' column='7'/>
    </data-member>
    <data-member access='public' layout-offset-in-bits='32'>
      <var-decl name='m1' type-id='type-id-2' visibility='default' filepath='../../abitests/test-struct0-v1.cc' line='4' column='12'/>
    </data-member>
    <data-member access='public' layout-offset-in-bits='64'>
      <var-decl name='m2' type-id='type-id-3' visibility='default' filepath='../../abitests/test-struct0-v1.cc' line='5' column='8'/>
    </data-member>
  </class-decl>
  <reference-type-def kind='lvalue' type-id='type-id-5' size-in-bits='64' alignment-in-bits='64' id='type-id-7'/>
  <function-decl name='foo' mangled-name='_Z3fooR2s0' filepath='../../abitests/test-struct0-v1.cc' line='10' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
    <parameter type-id='type-id-7'/>
    <return type-id='type-id-1'/>
  </function-decl>
</abi-instr>
