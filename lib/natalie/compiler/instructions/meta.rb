module Natalie
  class Compiler
    INSTRUCTIONS = [
      AliasGlobalInstruction,
      AliasMethodInstruction,
      ArrayConcatInstruction,
      ArrayPopInstruction,
      ArrayPopWithDefaultInstruction,
      ArrayPushInstruction,
      ArrayShiftInstruction,
      ArrayShiftWithDefaultInstruction,
      ArrayWrapInstruction,
      AutoloadConstInstruction,
      BreakInstruction,
      BreakOutInstruction,
      CaseEqualInstruction,
      CatchInstruction,
      CheckArgsInstruction,
      CheckExtraKeywordsInstruction,
      CheckRequiredKeywordsInstruction,
      ClassVariableGetInstruction,
      ClassVariableSetInstruction,
      ConstFindInstruction,
      ConstSetInstruction,
      ContinueInstruction,
      CreateArrayInstruction,
      CreateHashInstruction,
      CreateLambdaInstruction,
      DefineBlockInstruction,
      DefineClassInstruction,
      DefineMethodInstruction,
      DefineModuleInstruction,
      DupInstruction,
      DupObjectInstruction,
      DupRelInstruction,
      ElseInstruction,
      EndInstruction,
      GlobalVariableDefinedInstruction,
      GlobalVariableGetInstruction,
      GlobalVariableSetInstruction,
      HashDeleteInstruction,
      HashDeleteWithDefaultInstruction,
      HashMergeInstruction,
      HashPutInstruction,
      IfInstruction,
      InlineCppInstruction,
      InstanceVariableDefinedInstruction,
      InstanceVariableGetInstruction,
      InstanceVariableSetInstruction,
      IsDefinedInstruction,
      IsNilInstruction,
      LoadFileInstruction,
      MatchBreakPointInstruction,
      MatchExceptionInstruction,
      MethodDefinedInstruction,
      MoveRelInstruction,
      NextInstruction,
      NotInstruction,
      PatternArraySizeCheckInstruction,
      PopInstruction,
      PopKeywordArgsInstruction,
      PushArgInstruction,
      PushArgcInstruction,
      PushArgsInstruction,
      PushBlockInstruction,
      PushComplexInstruction,
      PushFalseInstruction,
      PushFloatInstruction,
      PushIntInstruction,
      PushLastMatchInstruction,
      PushNilInstruction,
      PushObjectClassInstruction,
      PushRangeInstruction,
      PushRationalInstruction,
      PushRegexpInstruction,
      PushRescuedInstruction,
      PushSelfInstruction,
      PushStringInstruction,
      PushSymbolInstruction,
      PushTrueInstruction,
      RedoInstruction,
      RetryInstruction,
      ReturnInstruction,
      SendInstruction,
      ShellInstruction,
      SingletonClassInstruction,
      StringAppendInstruction,
      StringToRegexpInstruction,
      SuperInstruction,
      SwapInstruction,
      ToArrayInstruction,
      ToHashInstruction,
      TryInstruction,
      UndefineMethodInstruction,
      VariableDeclareInstruction,
      VariableGetInstruction,
      VariableSetInstruction,
      WhileBodyInstruction,
      WhileInstruction,
      WithSingletonInstruction,
      YieldInstruction
    ].freeze
  end
end
