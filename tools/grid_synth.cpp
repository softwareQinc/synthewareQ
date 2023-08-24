/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2023 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <CLI/CLI.hpp>
#include <iostream>
#include <time.h>

#include "grid_synth/types.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/exact_synthesis.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using namespace std;

    bool check, details, verbose;
    real_t theta,eps;
    long int prec;
    domega_matrix_table_t s3_table;
    str_t tablefile = "";

    CLI::App app{"Grid Synthesis"};
  
    CLI::Option* theta_opt = app.add_option<real_t,float>("-t, --theta", theta, "Z-rotation angle in units of PI");
    CLI::Option* prec_opt = app.add_option<long int,int>("-p, --precision", prec, "Precision in based ten as a positive integer.");
    CLI::Option* read = app.add_option("-r, --read-table", tablefile, "Name of file containing s3 table");
    CLI::Option* write = app.add_option("-w, --write-table", tablefile, "Name of table file to write s3_table to.")
                           ->excludes(read);
    app.add_flag("-c, --check", check, "If set, program will output bool that will be 1 if the op string matches the input operator");
    app.add_flag("-d, --details", details, "If set, program will output the particular value of the approximation including the power of root two in the denominator and the true error");
    app.add_flag("-v, --verbose", verbose, "If set program will include additional output");
    
    CLI11_PARSE(app,argc,argv);
    DEFAULT_GMP_PREC = 4*prec+19;
    mpf_set_default_prec(log2(10)*DEFAULT_GMP_PREC);
    TOL = pow(real_t(10),-DEFAULT_GMP_PREC+2); 
    PI = gmp_pi();
    SQRT2 = sqrt(real_t(2));
    INV_SQRT2 = real_t(real_t(1) / SQRT2);
    HALF_INV_SQRT2 = real_t(real_t(1)/(real_t(2)*SQRT2));
    OMEGA = cplx_t(INV_SQRT2,INV_SQRT2);
    OMEGA_CONJ = cplx_t(INV_SQRT2,-INV_SQRT2);
    LOG_LAMBDA = log(LAMBDA.decimal());
    SQRT_LAMBDA = sqrt(LAMBDA.decimal());
    SQRT_LAMBDA_INV = sqrt(LAMBDA_INV.decimal());
    Im = cplx_t(real_t(0),real_t(1));
    eps = pow(real_t(10),-prec);
    
    //cout << "DEFAULT_GMP_PREC = " << DEFAULT_GMP_PREC << endl;
    //cout << scientific << setprecision(1) << "TOL = " << TOL << endl;
    //cout << fixed << setprecision(DEFAULT_GMP_PREC) << sin(theta*PI) << endl;
    //cout << "SQRT2 = " << SQRT2 << endl;
    //cout << "INV_SQRT2 = " << HALF_INV_SQRT2 << endl;
    //cout << "PI = " << PI << endl;

    //cout << "DEFAULT_GMP_PREC = " << DEFAULT_GMP_PREC << endl; 
    //cout << "TOL = " << TOL << endl;
    //cout << "PI = " << PI << endl;
    //cout << "SQRT2 = " << SQRT2 << endl;
    //cout << "INV_SQRT2 = " << INV_SQRT2 << endl;
    //cout << "HALF_INV_SQRT2 = " << HALF_INV_SQRT2 << endl;
    //cout << "OMEGA = " << OMEGA << endl;
    //cout << "OMEGA_CONJ = " << OMEGA_CONJ << endl;
    //cout << "Im = " << Im << endl;
    //cout << "eps = " << eps << endl;

    if(verbose) cout << "Finding approximation" << endl;

    if(*read) {
        if(verbose) cout << "Reading s3_table from " << tablefile << endl;
        s3_table = read_s3_table(tablefile);
    } else if(*write) {
        if(verbose) cout << "Generating new table file and writing to " << tablefile << endl; 
        s3_table = generate_s3_table();
        write_s3_table(tablefile, s3_table);
    } else if(ifstream(DEFAULT_TABLE_FILE)){
        if(verbose) cout << "Table file found at default location " << DEFAULT_TABLE_FILE << endl;
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if(verbose) cout << "Failed to find " << DEFAULT_TABLE_FILE 
                         << ". Generating new table file and writing to " 
                         << DEFAULT_TABLE_FILE<< endl; 
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE,s3_table);
    } 
    
    if(*prec_opt and *theta_opt) {
        random_numbers.seed(time(NULL));
        //RzApproximation rz_approx = find_rz_approximation(theta*PI, eps);
        RzApproximation rz_approx = find_fast_rz_approximation(theta*PI,eps);
        if(not rz_approx.solution_found()) {
          cout << "No approximation found for RzApproximation." << endl; 
          return 1;
        }
        if(verbose) cout << "Approximation Found" << endl;
        str_t op_str = synthesize(rz_approx.matrix(), s3_table);
        
        if(check) {
          cout << "Check flag = " << (rz_approx.matrix() == domega_matrix_from_str(full_simplify_str(op_str))) << endl;
        }

        if(details) {
          real_t scale = pow(SQRT2,rz_approx.matrix().k());
          cout << rz_approx.matrix() << endl;
          cout << "u decimal value = " << rz_approx.matrix().u().decimal().real()/scale << endl;
          cout << "t decimal value = " << rz_approx.matrix().t().decimal().real()/scale << endl;
          cout << "error = " << rz_approx.error() << endl;
        }
        
        for(auto &ch : full_simplify_str(op_str)) {
          cout << ch << " "; 
        }
        cout << endl;
    }

    return 0;
}
