<abi-instr version='1.0'>
  <type-decl name='int' size-in-bits='32' alignment-in-bits='32' id='type-id-1'/>
  <class-decl name='S0' visibility='default' is-declaration-only='yes' id='type-id-2'/>
  <class-decl name='S0' size-in-bits='32' alignment-in-bits='32' visibility='default' filepath='../../abitests/test-qual-type0-v0.cc' line='1' column='8' def-of-decl-id='type-id-2' id='type-id-3'>
    <member-type access='public'>
      <typedef-decl name='S0' type-id='type-id-2' filepath='../../abitests/test-qual-type0-v0.cc' line='2' column='1' id='type-id-4'/>
    </member-type>
    <data-member access='public' layout-offset-in-bits='0'>
      <var-decl name='m0' type-id='type-id-1' visibility='default' filepath='../../abitests/test-qual-type0-v0.cc' line='3' column='7'/>
    </data-member>
  </class-decl>
  <qualified-type-def type-id='type-id-3' const='yes' filepath='../../abitests/test-qual-type0-v0.cc' line='1' column='8' id='type-id-5'/>
  <reference-type-def kind='lvalue' type-id='type-id-5' size-in-bits='64' alignment-in-bits='64' id='type-id-6'/>
  <type-decl name='void' alignment-in-bits='8' id='type-id-7'/>
  <function-decl name='foo' mangled-name='_Z3fooRK2S0' filepath='../../abitests/test-qual-type0-v0.cc' line='7' column='1' visibility='default' binding='global' size-in-bits='8' alignment-in-bits='8'>
    <parameter type-id='type-id-6'/>
    <return type-id='type-id-7'/>
  </function-decl>
</abi-instr>
