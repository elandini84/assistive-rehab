/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file nlp.cpp
 * @authors: Ugo Pattacini <ugo.pattacini@iit.it>
 */

#include <limits>
#include <IpTNLP.hpp>
#include <IpIpoptApplication.hpp>
#include <yarp/math/Math.h>
#include "nlp.h"

using namespace std;
using namespace yarp::sig;
using namespace yarp::math;
using namespace assistive_rehab;


/****************************************************************/
class LimbOptimizerNLP : public Ipopt::TNLP
{
protected:
    const KeyPoint* k;
    const vector<double>& lengths;

    Ipopt::Index num_var;
    vector<pair<string,Vector>> result;

    /****************************************************************/
    bool get_nlp_info(Ipopt::Index &n, Ipopt::Index &m,
                      Ipopt::Index &nnz_jac_g,
                      Ipopt::Index &nnz_h_lag,
                      IndexStyleEnum &index_style) override
    {
        n=num_var;
        m=n/3;
        nnz_jac_g=3+6*(m-1);
        nnz_h_lag=(n*n+n)/2;
        index_style=TNLP::C_STYLE;
        return true;
    }

    /****************************************************************/
    bool get_bounds_info(Ipopt::Index n, Ipopt::Number *x_l,
                         Ipopt::Number *x_u, Ipopt::Index m,
                         Ipopt::Number *g_l, Ipopt::Number *g_u) override
    {
        for (Ipopt::Index i=0; i<n; i++)
        {
            x_l[i]=-numeric_limits<double>::infinity();
            x_u[i]=+numeric_limits<double>::infinity();;
        }
        for (Ipopt::Index i=0; i<m; i++)
        {
            g_l[i]=g_u[i]=lengths[i]*lengths[i];
        }
        return true;
    }

    /****************************************************************/
    bool get_starting_point(Ipopt::Index n, bool init_x,
                            Ipopt::Number *x, bool init_z,
                            Ipopt::Number *z_L, Ipopt::Number *z_U,
                            Ipopt::Index m, bool init_lambda,
                            Ipopt::Number *lambda) override
    {
        Ipopt::Index i=0;
        for (auto c=k->getChild(0); c!=nullptr; c=c->getChild(0))
        {
            Vector v=c->getPoint();
            x[i+0]=v[0];
            x[i+1]=v[1];
            x[i+2]=v[2];
            i+=3;
        }
        return true;
    }

    /****************************************************************/
    bool eval_f(Ipopt::Index n, const Ipopt::Number *x,
                bool new_x, Ipopt::Number &obj_value) override
    {
        Vector v(3);
        obj_value=0.0;
        Ipopt::Index i=0;
        Ipopt::Index j=0;
        for (auto c=k->getChild(0); c!=nullptr; c=c->getChild(0))
        {
            v[0]=x[i+0];
            v[1]=x[i+1];
            v[2]=x[i+2];
            obj_value+=norm2(v-c->getPoint());
            i+=3;
            j++;
        }
        return true;
    }

    /****************************************************************/
    bool eval_grad_f(Ipopt::Index n, const Ipopt::Number *x,
                     bool new_x, Ipopt::Number *grad_f) override
    {
        Ipopt::Index i=0;
        Ipopt::Index j=0;
        for (auto c=k->getChild(0); c!=nullptr; c=c->getChild(0))
        {
            Vector v=c->getPoint();
            grad_f[i+0]=2.0*(x[i+0]-v[0]);
            grad_f[i+1]=2.0*(x[i+1]-v[1]);
            grad_f[i+2]=2.0*(x[i+2]-v[2]);
            i+=3;
            j++;
        }
        return true;
    }

    /****************************************************************/
    bool eval_g(Ipopt::Index n, const Ipopt::Number *x,
                bool new_x, Ipopt::Index m, Ipopt::Number *g) override
    {
        Vector v(3);
        Vector b=k->getPoint();
        for (Ipopt::Index i=0; i<m; i++)
        {
            Ipopt::Index j=3*i;
            v[0]=x[j+0];
            v[1]=x[j+1];
            v[2]=x[j+2];
            g[i]=norm2(v-b);
            b=v;
        }
        return true;
    }

    /****************************************************************/
    bool eval_jac_g(Ipopt::Index n, const Ipopt::Number *x,
                    bool new_x, Ipopt::Index m, Ipopt::Index nele_jac,
                    Ipopt::Index *iRow, Ipopt::Index *jCol,
                    Ipopt::Number *values) override
    {
        if (values==nullptr)
        {
            iRow[0]=0; jCol[0]=0;
            iRow[1]=0; jCol[1]=1;
            iRow[2]=0; jCol[2]=2;

            Ipopt::Index i=3;
            for (Ipopt::Index j=1; j<m; j++)
            {
                Ipopt::Index k=j-1;
                iRow[i+0]=j; jCol[i+0]=k+0;
                iRow[i+1]=j; jCol[i+1]=k+1;
                iRow[i+2]=j; jCol[i+2]=k+2;
                iRow[i+3]=j; jCol[i+3]=k+3;
                iRow[i+4]=j; jCol[i+4]=k+4;
                iRow[i+5]=j; jCol[i+5]=k+5;
                i+=6;
            }
        }
        else
        {
            Vector b=k->getPoint();
            values[0]=2.0*(x[0]-b[0]);
            values[1]=2.0*(x[1]-b[1]);
            values[2]=2.0*(x[2]-b[2]);

            Ipopt::Index i=3;
            Ipopt::Index j=0;
            for (Ipopt::Index k=1; k<m; k++)
            {
                values[i+0]=-2.0*(x[j+3]-x[j+0]);
                values[i+1]=-2.0*(x[j+4]-x[j+1]);
                values[i+2]=-2.0*(x[j+5]-x[j+2]);
                values[i+3]=-values[i+0];
                values[i+4]=-values[i+1];
                values[i+5]=-values[i+2];
                i+=6;
                j+=3;
            }
        }
        return true;
    }

    /****************************************************************/
    bool eval_h(Ipopt::Index n, const Ipopt::Number *x,
                bool new_x, Ipopt::Number obj_factor,
                Ipopt::Index m, const Ipopt::Number *lambda,
                bool new_lambda, Ipopt::Index nele_hess,
                Ipopt::Index *iRow, Ipopt::Index *jCol,
                Ipopt::Number *values) override
    {
        if (values==nullptr)
        {
            Ipopt::Index i=0;
            for (Ipopt::Index r=0; r<n; r++)
            {
                for (Ipopt::Index c=0; c<=r; c++)
                {
                    iRow[i]=r;
                    jCol[i]=c;
                    i++;
                }
            }
        }
        else
        {
            Ipopt::Index i=0;
            for (Ipopt::Index r=0; r<n; r++)
            {
                for (Ipopt::Index c=0; c<=r; c++)
                {
                    values[i]=0.0;
                    if (r==c)
                    {
                        Ipopt::Index j=r/3;
                        values[i]+=2.0*(obj_factor+lambda[j]);
                        if (j+1<m)
                        {
                            values[i]+=2.0*lambda[j+1];
                        }
                    }
                    else for (Ipopt::Index j=1; j<m; j++)
                    {
                        Ipopt::Index lb=3*j-3;
                        Ipopt::Index hb=lb+5;
                        if ((r>=lb) && (r<=hb) && (c>=lb) && (c<=hb) && (abs(r-c)==3))
                        {
                            values[i]-=2.0*lambda[j];
                        }
                    }
                    i++;
                }
            }
        }
        return true;
    }

    /****************************************************************/
    void finalize_solution(Ipopt::SolverReturn status,
                           Ipopt::Index n, const Ipopt::Number *x,
                           const Ipopt::Number *z_L,
                           const Ipopt::Number *z_U,
                           Ipopt::Index m, const Ipopt::Number *g,
                           const Ipopt::Number *lambda,
                           Ipopt::Number obj_value,
                           const Ipopt::IpoptData *ip_data,
                           Ipopt::IpoptCalculatedQuantities *ip_cq) override
    {
        Vector v(3);
        auto c=k->getChild(0); 
        for (Ipopt::Index i=0; i<n; i+=3)
        {
            v[0]=x[i+0];
            v[1]=x[i+1];
            v[2]=x[i+2];
            result.push_back(make_pair(c->getTag(),v));
            c=c->getChild(0);
        }
    }

public:
    /****************************************************************/
    LimbOptimizerNLP(const KeyPoint* k_, const vector<double>& lengths_) :
                  k(k_), lengths(lengths_)
    {
        num_var=0;
        for (auto c=k->getChild(0); c!=nullptr; c=c->getChild(0))
        {
            num_var+=3;
        }
    }

    /****************************************************************/
    vector<pair<string,Vector>> get_result() const
    {
        return result;
    }
};


/****************************************************************/
vector<pair<string,Vector>> LimbOptimizer::optimize(const KeyPoint* k,
                                                    const vector<double>& lengths)
{
    Ipopt::SmartPtr<Ipopt::IpoptApplication> app=new Ipopt::IpoptApplication;
    app->Options()->SetNumericValue("tol",1e-4);
    app->Options()->SetNumericValue("constr_viol_tol",1e-4);
    app->Options()->SetIntegerValue("acceptable_iter",0);
    app->Options()->SetStringValue("mu_strategy","adaptive");
    app->Options()->SetIntegerValue("max_iter",100);
    app->Options()->SetNumericValue("max_cpu_time",0.05);
    app->Options()->SetStringValue("hessian_constant","yes");
    app->Options()->SetIntegerValue("print_level",0);
    app->Initialize();

    Ipopt::SmartPtr<LimbOptimizerNLP> nlp=new LimbOptimizerNLP(k,lengths);
    Ipopt::ApplicationReturnStatus status=app->OptimizeTNLP(GetRawPtr(nlp));
    return nlp->get_result();
}
