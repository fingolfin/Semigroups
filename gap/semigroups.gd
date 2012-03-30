############################################################################# 
## 
#W  closure.gd
#Y  Copyright (C) 2011-12                                 James D. Mitchell
## 
##  Licensing information can be found in the README file of this package. 
## 
############################################################################# 
##

DeclareGlobalFunction("ClosureInverseSemigroup");
DeclareGlobalFunction("ClosureInverseSemigroupNC");
DeclareGlobalFunction("ClosureSemigroup");
DeclareGlobalFunction("ClosureSemigroupNC");
DeclareAttribute("GeneratorsOfInverseSemigroup", IsInverseSemigroup);
DeclareGlobalFunction("InverseMonoid");
DeclareGlobalFunction("InverseSemigroup");
DeclareOperation("InverseMonoidByGenerators", [IsPartialPermCollection]);
DeclareOperation("InverseSemigroupByGenerators", [IsPartialPermCollection, IsPartialPermCollection]);
DeclareGlobalFunction("SingularSemigroup");
DeclareGlobalFunction("SymmetricInverseSemigp");
