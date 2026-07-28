* =====================================================
* 2D Radiation Collection Optimization
* ChatGPT helped in the creation of this script, it would have been way uglier without it -VV
* =====================================================

Sets
    iu /u1*u20/
    iv /v1*v20/
    ia /a1*a20/;

Scalars
    d      /5/
    Area   /20/
    Nu     /20/
    Nv     /20/
    Nz     /20/;
*Note Nz was originally indeed Na, but that is function in GAMS so I changed it to Nz

Variables
    x
    y
    z;

Positive Variables x, y;

Equations
    volume
    objective;

* -----------------------------------------------------
* Volume constraint
* -----------------------------------------------------

volume..
    x*y =e= Area;

* -----------------------------------------------------
* Objective function
* -----------------------------------------------------

objective..

z =e=

sum(iu,

sum(iv,

sum(ia,

(d + ((ord(iu)-0.5)*(x/Nu)))/((sqr(d + ((ord(iu)-0.5)*(x/Nu)))+sqr((-y/2 + (ord(iv)-0.5)*(y/Nv))-(-0.5 + (ord(ia)-0.5)*(1/Nz))))**1.5)*(x/Nu)* (y/Nv)* (1/Nz)

)))

;

* -----------------------------------------------------
* Bounds
* -----------------------------------------------------

x.lo = 0.1;
y.lo = 0.5;

* -----------------------------------------------------
* Initial guesses
* -----------------------------------------------------

x.l = 5;
y.l = 4;

* -----------------------------------------------------
* Model
* -----------------------------------------------------

Model detector_model /all/;

Solve detector_model using NLP maximizing z;

* -----------------------------------------------------
* Results
* -----------------------------------------------------

Display x.l, y.l, z.l;

* =====================================================
* PARAMETRIC SWEEP + CSV EXPORT
*
* Add this AFTER the solve statement
* =====================================================

Sets
    ix /x1*x40/;

Parameters
    xsweep(ix)
    yresult(ix)
    zresult(ix);

* -----------------------------------------------------
* Generate x values to test
* -----------------------------------------------------

xsweep(ix) = 0.5 + 0.5*ord(ix);

* -----------------------------------------------------
* Sweep over x values
* -----------------------------------------------------

loop(ix,

*Fix x
    x.fx = xsweep(ix);

*Good initial guess for y
    y.l = Area / xsweep(ix);

*Solve
    solve detector_model using NLP maximizing z;

*Store results
    yresult(ix) = y.l;
    zresult(ix) = z.l;

);

* -----------------------------------------------------
* Export to CSV
* -----------------------------------------------------

file fout /detector_resultsd5.csv/;
put fout;

put "x,y,z" /;

loop(ix,

    put
        xsweep(ix):12:6, ",",
        yresult(ix):12:6, ",",
        zresult(ix):12:6
    /

);

putclose fout;