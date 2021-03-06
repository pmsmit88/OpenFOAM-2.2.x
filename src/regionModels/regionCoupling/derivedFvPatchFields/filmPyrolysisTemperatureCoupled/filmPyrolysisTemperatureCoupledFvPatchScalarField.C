/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "filmPyrolysisTemperatureCoupledFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"
#include "pyrolysisModel.H"
#include "surfaceFilmModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::
filmPyrolysisTemperatureCoupledFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(p, iF),
    phiName_("phi"),
    rhoName_("rho")
{}


Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::
filmPyrolysisTemperatureCoupledFvPatchScalarField
(
    const filmPyrolysisTemperatureCoupledFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchScalarField(ptf, p, iF, mapper),
    phiName_(ptf.phiName_),
    rhoName_(ptf.rhoName_)
{}


Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::
filmPyrolysisTemperatureCoupledFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchScalarField(p, iF),
    phiName_(dict.lookupOrDefault<word>("phi", "phi")),
    rhoName_(dict.lookupOrDefault<word>("rho", "rho"))
{
    fvPatchScalarField::operator=(scalarField("value", dict, p.size()));
}


Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::
filmPyrolysisTemperatureCoupledFvPatchScalarField
(
    const filmPyrolysisTemperatureCoupledFvPatchScalarField& fptpsf
)
:
    fixedValueFvPatchScalarField(fptpsf),
    phiName_(fptpsf.phiName_),
    rhoName_(fptpsf.rhoName_)
{}


Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::
filmPyrolysisTemperatureCoupledFvPatchScalarField
(
    const filmPyrolysisTemperatureCoupledFvPatchScalarField& fptpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(fptpsf, iF),
    phiName_(fptpsf.phiName_),
    rhoName_(fptpsf.rhoName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    typedef regionModels::surfaceFilmModels::surfaceFilmModel filmModelType;
    typedef regionModels::pyrolysisModels::pyrolysisModel pyrModelType;

    // Since we're inside initEvaluate/evaluate there might be processor
    // comms underway. Change the tag we use.
    int oldTag = UPstream::msgType();
    UPstream::msgType() = oldTag+1;

    bool filmOk =
        db().time().foundObject<filmModelType>("surfaceFilmProperties");


    bool pyrOk = db().time().foundObject<pyrModelType>("pyrolysisProperties");

    if (!filmOk || !pyrOk)
    {
        // do nothing on construction - film model doesn't exist yet
        return;
    }

    scalarField& Tp = *this;

    const label patchI = patch().index();

    // Retrieve film model
    const filmModelType& filmModel =
        db().time().lookupObject<filmModelType>("surfaceFilmProperties");

    const label filmPatchI = filmModel.regionPatchID(patchI);

    scalarField alphaFilm = filmModel.alpha().boundaryField()[filmPatchI];
    filmModel.toPrimary(filmPatchI, alphaFilm);

    scalarField TFilm = filmModel.Ts().boundaryField()[filmPatchI];
    filmModel.toPrimary(filmPatchI, TFilm);

    // Retrieve pyrolysis model
    const pyrModelType& pyrModel =
        db().time().lookupObject<pyrModelType>("pyrolysisProperties");

    const label pyrPatchI = pyrModel.regionPatchID(patchI);

    scalarField TPyr = pyrModel.T().boundaryField()[pyrPatchI];
    pyrModel.toPrimary(pyrPatchI, TPyr);


    // Evaluate temperature
    Tp = alphaFilm*TFilm + (1.0 - alphaFilm)*TPyr;

    // Restore tag
    UPstream::msgType() = oldTag;

    fixedValueFvPatchScalarField::updateCoeffs();
}


void Foam::filmPyrolysisTemperatureCoupledFvPatchScalarField::write
(
    Ostream& os
) const
{
    fvPatchScalarField::write(os);
    writeEntryIfDifferent<word>(os, "phi", "phi", phiName_);
    writeEntryIfDifferent<word>(os, "rho", "rho", rhoName_);
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        filmPyrolysisTemperatureCoupledFvPatchScalarField
    );
}


// ************************************************************************* //
