/*
* Copyright 2026 ZXing authors
* Based on PDF417 Reed-Solomon decoder
*/
// SPDX-License-Identifier: Apache-2.0

#include "ReedSolomonModulusDecoder.h"

#include "pdf417/PDFModulusGF.h"
#include "pdf417/PDFModulusPoly.h"
#include "ZXAlgorithms.h"

namespace ZXing {

using namespace Pdf417;

static bool RunEuclideanAlgorithm(const ModulusGF& field, ModulusPoly a, ModulusPoly b, int R, ModulusPoly& sigma, ModulusPoly& omega)
{
	// Assume a's degree is >= b's
	if (a.degree() < b.degree()) {
		swap(a, b);
	}

	ModulusPoly rLast = a;
	ModulusPoly r = b;
	ModulusPoly tLast = field.zero();
	ModulusPoly t = field.one();

	// Run Euclidean algorithm until r's degree is less than R/2
	while (r.degree() >= R / 2) {
		ModulusPoly rLastLast = rLast;
		ModulusPoly tLastLast = tLast;
		rLast = r;
		tLast = t;

		// Divide rLastLast by rLast, with quotient in q and remainder in r
		if (rLast.isZero()) {
			// Oops, Euclidean algorithm already terminated?
			return false;
		}
		r = rLastLast;
		ModulusPoly q = field.zero();
		int denominatorLeadingTerm = rLast.coefficient(rLast.degree());
		int dltInverse = field.inverse(denominatorLeadingTerm);
		while (r.degree() >= rLast.degree() && !r.isZero()) {
			int degreeDiff = r.degree() - rLast.degree();
			int scale = field.multiply(r.coefficient(r.degree()), dltInverse);
			q = q.add(field.buildMonomial(degreeDiff, scale));
			r = r.subtract(rLast.multiplyByMonomial(degreeDiff, scale));
		}

		t = q.multiply(tLast).subtract(tLastLast).negative();
	}

	int sigmaTildeAtZero = t.coefficient(0);
	if (sigmaTildeAtZero == 0) {
		return false;
	}

	int inverse = field.inverse(sigmaTildeAtZero);
	sigma = t.multiply(inverse);
	omega = r.multiply(inverse);
	return true;
}

static bool FindErrorLocations(const ModulusGF& field, const ModulusPoly& errorLocator, std::vector<int>& result)
{
	// This is a direct application of Chien's search
	int numErrors = errorLocator.degree();
	result.resize(numErrors);
	int e = 0;
	for (int i = 1; i < field.size() && e < numErrors; i++) {
		if (errorLocator.evaluateAt(i) == 0) {
			result[e] = field.inverse(i);
			e++;
		}
	}
	return e == numErrors;
}

static std::vector<int> FindErrorMagnitudes(const ModulusGF& field, const ModulusPoly& errorEvaluator, const ModulusPoly& errorLocator, const std::vector<int>& errorLocations)
{
	int errorLocatorDegree = errorLocator.degree();
	std::vector<int> formalDerivativeCoefficients(errorLocatorDegree);
	for (int i = 1; i <= errorLocatorDegree; i++) {
		formalDerivativeCoefficients[errorLocatorDegree - i] = field.multiply(i, errorLocator.coefficient(i));
	}

	ModulusPoly formalDerivative(field, formalDerivativeCoefficients);
	// This is directly applying Forney's Formula
	std::vector<int> result(errorLocations.size());
	for (size_t i = 0; i < result.size(); i++) {
		int xiInverse = field.inverse(errorLocations[i]);
		int numerator = field.subtract(0, errorEvaluator.evaluateAt(xiInverse));
		int denominator = field.inverse(formalDerivative.evaluateAt(xiInverse));
		result[i] = field.multiply(numerator, denominator);
	}
	return result;
}

/**
* Reed-Solomon error correction decoder for prime fields.
*
* @param field The ModulusGF field to use
* @param received Received codewords (modified in place with corrections)
* @param numECCodewords Number of error correction codewords
* @param nbErrors Output: number of errors corrected
* @return true if successful, false if too many errors
*/
bool ReedSolomonDecodeModulus(const ModulusGF& field, std::vector<int>& received, int numECCodewords, int& nbErrors)
{
	ModulusPoly poly(field, received);
	std::vector<int> S(numECCodewords);
	bool error = false;
	for (int i = numECCodewords; i > 0; i--) {
		int eval = poly.evaluateAt(field.exp(i));
		S[numECCodewords - i] = eval;
		if (eval != 0) {
			error = true;
		}
	}

	if (!error) {
		nbErrors = 0;
		return true;
	}

	ModulusPoly syndrome(field, S);

	ModulusPoly sigma, omega;
	if (!RunEuclideanAlgorithm(field, field.buildMonomial(numECCodewords, 1), syndrome, numECCodewords, sigma, omega)) {
		return false;
	}

	std::vector<int> errorLocations;
	if (!FindErrorLocations(field, sigma, errorLocations)) {
		return false;
	}

	std::vector<int> errorMagnitudes = FindErrorMagnitudes(field, omega, sigma, errorLocations);

	int receivedSize = Size(received);
	for (size_t i = 0; i < errorLocations.size(); i++) {
		int position = receivedSize - 1 - field.log(errorLocations[i]);
		if (position < 0) {
			return false;
		}
		received[position] = field.subtract(received[position], errorMagnitudes[i]);
	}
	nbErrors = Size(errorLocations);
	return true;
}

} // ZXing
