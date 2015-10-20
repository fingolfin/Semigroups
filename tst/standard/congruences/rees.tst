###########################################################################
##
#W  standard/congruences/rees.tst
#Y  Copyright (C) 2015                                      Michael Torpey
##
##  Licensing information can be found in the README file of this package.
##
#############################################################################
##
gap> START_TEST("Semigroups package: standard/congruences/rees.tst");
gap> LoadPackage("semigroups", false);;

# Set info levels and user preferences
gap> SEMIGROUPS_StartTest();

#T# ReesCongTest1
# Test whether a congruence is Rees and find its ideal
gap> S := Semigroup([Transformation([2, 3, 4, 3, 1, 1]),
>                    Transformation([6, 4, 4, 4, 6, 1])]);;
gap> I := SemigroupIdeal(S, Transformation([4, 4, 4, 4, 4, 2]),
>                           Transformation([3, 3, 3, 3, 3, 2]));;
gap> cong := SemigroupCongruence(S,
>  [[Transformation([4, 4, 4, 4, 4, 2]), Transformation([4, 4, 4, 4, 4, 4])],
>   [Transformation([3, 3, 3, 3, 3, 2]), Transformation([4, 4, 4, 4, 4, 4])],
>   [Transformation([4, 3, 3, 3, 4, 3]), Transformation([4, 4, 4, 4, 4, 4])],
>   [Transformation([3, 3, 3, 3, 4, 4]), Transformation([4, 4, 4, 4, 4, 4])],
>   [Transformation([4, 4, 4, 4, 4, 3]), Transformation([3, 3, 3, 3, 3, 3])]]);;
gap> IsReesCongruence(cong);
true
gap> SemigroupIdealOfReesCongruence(cong) = I;
true
gap> cong := SemigroupCongruence(S, []);;
gap> IsReesCongruence(cong);
false
gap> S := Semigroup([PartialPerm([1, 2, 3], [1, 2, 3]),
>                    PartialPerm([1, 2, 3, 4], [2, 4, 3, 5])]);;
gap> cong := SemigroupCongruence(S, []);;
gap> IsReesCongruence(cong);
true
gap> cong := SemigroupCongruence(S, [PartialPerm([1, 2, 3], [2, 4, 3]),
>                                    PartialPerm([1, 2, 3, 4], [2, 4, 3, 5])]);;
gap> IsReesCongruence(cong);
false

#T# ReesCongTest2
# Create a congruence, calculate its congruence classes and try some operations
gap> S := Semigroup([Transformation([2, 4, 3, 5, 5]),
>                    Transformation([3, 1, 1, 4, 4]),
>                    Transformation([3, 1, 4, 2, 4]),
>                    Transformation([3, 4, 2, 3, 4]),
>                    Transformation([4, 1, 5, 1, 2])]);
<transformation semigroup of degree 5 with 5 generators>
gap> I := SemigroupIdeal(S, [Transformation([3, 1, 1, 4, 4]),
>                            Transformation([1, 4, 1, 4, 1])]);;
gap> cong := ReesCongruenceOfSemigroupIdeal(I);;
gap> NrCongruenceClasses(cong);
19
gap> cc := Set(CongruenceClasses(cong));;
gap> Size(cc);
19
gap> List(cc, Size);
[ 1095, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ]
gap> cc[1] * cc[1];
{Transformation( [ 3, 1, 1, 4, 4 ] )}
gap> cc[7] * cc[1];
{Transformation( [ 3, 1, 1, 4, 4 ] )}
gap> cc[2] * cc[5];
{Transformation( [ 2, 4, 1, 3, 1 ] )}
gap> cc[9] * cc[7] = cc[11];
true

#T# ReesCongTest3
# Convert a congruence to generating pairs
gap> S := Semigroup([Transformation([1, 3, 2, 4, 3]),
>                      Transformation([1, 3, 5, 5, 3]),
>                      Transformation([5, 1, 2, 5, 5])]);;
gap> I := SemigroupIdeal(S, Transformation([5, 2, 1, 5, 2]),
>                            Transformation([5, 2, 1, 5, 2]));;
gap> cong := ReesCongruenceOfSemigroupIdeal(I);;
gap> ccong := AsSemigroupCongruenceByGeneratingPairs(cong);
<semigroup congruence over <transformation semigroup of size 61, degree 5 
 with 3 generators> with 1 generating pairs>
gap> NrCongruenceClasses(ccong);
12
gap> IsReesCongruence(ccong);
true
gap> SemigroupIdealOfReesCongruence(ccong) = I;
true

#T# ReesCongTest4
# Test the \in function
gap> S := Semigroup([Transformation([2, 4, 3, 5, 5]),
>                    Transformation([3, 1, 1, 4, 4]),
>                    Transformation([3, 1, 4, 2, 4]),
>                    Transformation([3, 4, 2, 3, 4]),
>                    Transformation([4, 1, 5, 1, 2])]);;
gap> I := SemigroupIdeal(S, [Transformation([3, 1, 1, 4, 4]),
>                            Transformation([1, 4, 1, 4, 1])]);;
gap> cong := ReesCongruenceOfSemigroupIdeal(I);;
gap> x := Transformation([3, 4, 2, 4]);;      # not in I
gap> y := Transformation([1, 5, 5, 5, 4]);;   # in I
gap> z := Transformation([5, 5, 1, 1, 3]);;   # not even in S
gap> [x, y] in cong;
false
gap> [x, x] in cong;
true
gap> [x, y, y] in cong;
Error, Semigroups: in: usage,
the first arg <pair> must be a list of length 2,
gap> [x, z] in cong;
Error, Semigroups: in: usage,
the elements of 1st arg <pair> must be in the range of 2nd arg <cong>,
gap> t := Transformation([1, 3, 4, 1, 4]);;   # in i
gap> [t, y] in cong;
true
gap> [x, x] in cong;
true
gap> im := ImagesElm(cong, t);;
gap> Size(im) = Size(I);
true
gap> ForAll(im, x -> x in I);
true
gap> im := ImagesElm(cong, x);;
gap> Size(im);
1
gap> ImagesElm(cong, z);
Error, Semigroups: ImagesElm: usage,
the args <cong> and <elm> must refer to the same semigroup,
gap> yclass := CongruenceClassOfElement(cong, y);;
gap> x in yclass;
false
gap> tclass := CongruenceClassOfElement(cong, t);;
gap> y in tclass;
true
gap> CongruenceClassOfElement(cong, z);
Error, Semigroups: EquivalenceClassOfElement: usage,
the second arg <elm> must be in the semigroup of first arg <cong>,
gap> xclass := CongruenceClassOfElement(cong, x);
{Transformation( [ 3, 4, 2, 4 ] )}
gap> x in xclass;
true
gap> xclass * yclass = tclass;
true
gap> yclass * xclass = yclass;
true
gap> xxclass := CongruenceClassOfElement(cong, x * x);;
gap> xclass * xclass = xxclass;
true

#T# ReesCongTest5
# Join some congruences together
gap> S := Semigroup([Transformation([1, 1, 3, 1, 3]),
>                      Transformation([2, 1, 2, 2, 2]),
>                      Transformation([3, 1, 3, 2, 4])]);;
gap> I := SemigroupIdeal(S, Transformation([1, 1, 1, 3, 1]));;
gap> J := SemigroupIdeal(S, Transformation([3, 3, 3, 3, 1]));;
gap> ci := ReesCongruenceOfSemigroupIdeal(I);;
gap> cj := ReesCongruenceOfSemigroupIdeal(J);;
gap> cc := JoinSemigroupCongruences(ci, cj);;
gap> NrCongruenceClasses(ci); NrCongruenceClasses(cj); NrCongruenceClasses(cc);
16
17
15
gap> K := SemigroupIdeal(FullTransformationMonoid(5),
>                         Transformation([3, 2, 5, 4, 2]));;
gap> ck := ReesCongruenceOfSemigroupIdeal(K);;
gap> JoinSemigroupCongruences(ci, ck);
Error, Semigroups: JoinSemigroupCongruences: usage,
the args <c1> and <c2> must be congruences of the same semigroup,

#E#
gap> STOP_TEST("Semigroups package: standard/congruences/rees.tst");