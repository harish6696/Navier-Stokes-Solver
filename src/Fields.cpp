#include "Fields.hpp"

#include <algorithm>
#include <iostream>
#include <math.h>

Fields::Fields(Grid &grid, double nu, double dt, double tau, double UI, double VI, double PI, double GX, double GY)
    : _nu(nu), _dt(dt), _tau(tau), _gx(GX), _gy(GY) {

    _U = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);
    _V = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);
    _P = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);

    _F = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);
    _G = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);
    _RS = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);

    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();

        _U(i, j) = UI;
        _V(i, j) = VI;
        _P(i, j) = PI;
    }
}

Fields::Fields(Grid &grid, double nu, double alpha, double beta, double dt, double tau, double UI, double VI, double PI,
               double TI, double GX, double GY)
    : _nu(nu), _alpha(alpha), _beta(beta), _dt(dt), _tau(tau), _gx(GX), _gy(GY) {

    _U = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);
    _V = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);
    _P = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);
    _T = Matrix<double>(grid.imax() + 2, grid.jmax() + 2);

    _F = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);
    _G = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);
    _RS = Matrix<double>(grid.imax() + 2, grid.jmax() + 2, 0.0);

    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();

        _U(i, j) = UI;
        _V(i, j) = VI;
        _P(i, j) = PI;
        _T(i, j) = TI;
    }
}

void Fields::calculate_temperatures(Grid &grid) {

    // Temporary matrix to store temperature
    Matrix<double> T_new(grid.imax() + 2, grid.jmax() + 2, 0.0);

    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();
        T_new(i, j) = _T(i, j) + _dt * (-Discretization::convection_t(_U, _V, _T, i, j) +
                                        _alpha * Discretization::laplacian(_T, i, j));
    }
    _T = T_new;
}

void Fields::calculate_fluxes(Grid &grid, bool energy_eq) {
    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();

        _F(i, j) = _U(i, j) + _dt * ((_nu * Discretization::laplacian(_U, i, j)) -
                                     Discretization::convection_u(_U, _V, i, j) + (1 - energy_eq) * _gx);

        _G(i, j) = _V(i, j) + _dt * ((_nu * Discretization::laplacian(_V, i, j)) -
                                     Discretization::convection_v(_U, _V, i, j) + (1 - energy_eq) * _gy);

        if (energy_eq) {
            _F(i, j) -= _gx * _dt * (_beta * 0.5 * (_T(i, j) + _T(i + 1, j)));
            _G(i, j) -= _gy * _dt * (_beta * 0.5 * (_T(i, j) + _T(i, j + 1)));
        }
    }

    // Applying Flux BC to fixed walls
    for (auto &elem : grid.fixed_wall_cells()) {
        int i = elem->i();
        int j = elem->j();

        if (elem->is_border(border_position::TOP)) {
            if (elem->is_border(border_position::RIGHT)) {
                _F(i, j) = 0.0;
                _G(i, j) = 0.0;
            }

            else if (elem->is_border(border_position::LEFT)) {
                _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                _G(i, j) = 0.0;
            }

            else if (elem->is_border(border_position::BOTTOM)) {
                _G(i, j) = 0.0;
                _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
            }

            else {
                _G(i, j) = _V(i, j);
            }

        }

        else if (elem->is_border(border_position::BOTTOM)) {
            if (elem->is_border(border_position::RIGHT)) {
                _F(i, j) = 0.0;
                _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
            }

            else if (elem->is_border(border_position::LEFT)) {
                _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
            }

            else {

                _G(i, elem->neighbour(border_position::BOTTOM)->j()) =
                    _V(i, elem->neighbour(border_position::BOTTOM)->j());
            }
        }

        else if (elem->is_border(border_position::RIGHT)) {
            if (elem->is_border(border_position::LEFT)) {
                _F(i, j) = 0.0;
                _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
            }

            else {
                _F(i, j) = _U(i, j);
            }
        }

        else if (elem->is_border(border_position::LEFT)) {
            _F(elem->neighbour(border_position::LEFT)->i(), j) = _U(elem->neighbour(border_position::LEFT)->i(), j);
        }
    }

    if (energy_eq) {
        /***************************************/
        // For cold fixed_wall_cells
        /***************************************/
        for (auto &elem : grid.cold_fixed_wall_cells()) {
            int i = elem->i();
            int j = elem->j();

            if (elem->is_border(border_position::TOP)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::BOTTOM)) {
                    _G(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, j) = _V(i, j);
                }
            }

            else if (elem->is_border(border_position::BOTTOM)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) =
                        _V(i, elem->neighbour(border_position::BOTTOM)->j());
                }
            }

            else if (elem->is_border(border_position::RIGHT)) {
                if (elem->is_border(border_position::LEFT)) {
                    _F(i, j) = 0.0;
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                }

                else {
                    _F(i, j) = _U(i, j);
                }
            }

            else if (elem->is_border(border_position::LEFT)) {
                _F(elem->neighbour(border_position::LEFT)->i(), j) = _U(elem->neighbour(border_position::LEFT)->i(), j);
            }
        }

        /***************************************/
        // For hot fixed_wall_cells
        /***************************************/
        for (auto &elem : grid.hot_fixed_wall_cells()) {
            int i = elem->i();
            int j = elem->j();

            if (elem->is_border(border_position::TOP)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::BOTTOM)) { // Need to verify
                    _G(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, j) = _V(i, j);
                }

            }

            else if (elem->is_border(border_position::BOTTOM)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) =
                        _V(i, elem->neighbour(border_position::BOTTOM)->j());
                }
            }

            else if (elem->is_border(border_position::RIGHT)) {
                if (elem->is_border(border_position::LEFT)) {
                    _F(i, j) = 0.0;
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                }

                else {
                    _F(i, j) = _U(i, j);
                }
            }

            else if (elem->is_border(border_position::LEFT)) {
                _F(elem->neighbour(border_position::LEFT)->i(), j) = _U(elem->neighbour(border_position::LEFT)->i(), j);
            }
        }

        /***************************************/
        // For adiabatic fixed_wall_cells
        /***************************************/
        for (auto &elem : grid.adiabatic_fixed_wall_cells()) {
            int i = elem->i();
            int j = elem->j();

            if (elem->is_border(border_position::TOP)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, j) = 0.0;
                }

                else if (elem->is_border(border_position::BOTTOM)) {
                    _G(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, j) = _V(i, j);
                }

            }

            else if (elem->is_border(border_position::BOTTOM)) {
                if (elem->is_border(border_position::RIGHT)) {
                    _F(i, j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else if (elem->is_border(border_position::LEFT)) {
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) = 0.0;
                }

                else {
                    _G(i, elem->neighbour(border_position::BOTTOM)->j()) =
                        _V(i, elem->neighbour(border_position::BOTTOM)->j());
                }
            }

            else if (elem->is_border(border_position::RIGHT)) {
                if (elem->is_border(border_position::LEFT)) {
                    _F(i, j) = 0.0;
                    _F(elem->neighbour(border_position::LEFT)->i(), j) = 0.0;

                }

                else {
                    _F(i, j) = _U(i, j);
                }
            }

            else if (elem->is_border(border_position::LEFT)) {
                _F(elem->neighbour(border_position::LEFT)->i(), j) = _U(elem->neighbour(border_position::LEFT)->i(), j);
            }
        }
    }

    // Flux setup for Moving wall cells
    for (auto &elem : grid.moving_wall_cells()) {
        int i = elem->i();
        int j = elem->j();

        _G(i, elem->neighbour(border_position::BOTTOM)->j()) = _V(i, elem->neighbour(border_position::BOTTOM)->j());
    }

    // Flux setup for inflow cells
    for (auto &elem : grid.inflow_cells()) {
        int i = elem->i();
        int j = elem->j();

        _F(i, j) = _U(i, j);
    }

    // Flux setup for outflow cells
    for (auto &elem : grid.outflow_cells()) {
        int i = elem->i();
        int j = elem->j();

        _F(elem->neighbour(border_position::LEFT)->i(), j) = _U(elem->neighbour(border_position::LEFT)->i(), j);
    }
}

void Fields::calculate_rs(Grid &grid) {
    auto idt = 1. / _dt;
    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();
        rs(i, j) = idt * (((_F(i, j) - _F(elem->neighbour(border_position::LEFT)->i(), j)) / grid.dx()) +
                          ((_G(i, j) - _G(i, elem->neighbour(border_position::BOTTOM)->j())) / grid.dy()));
    }
}

void Fields::calculate_velocities(Grid &grid) {

    for (const auto &elem : grid.fluid_cells()) {
        int i = elem->i();
        int j = elem->j();

        _U(i, j) = _F(i, j) - (_dt / grid.dx()) * (_P(elem->neighbour(border_position::RIGHT)->i(), j) - _P(i, j));

        _V(i, j) = _G(i, j) - (_dt / grid.dy()) * (_P(i, elem->neighbour(border_position::TOP)->j()) - _P(i, j));
    }
}

double Fields::calculate_dt(Grid &grid) {

    auto max_u = 0.0;
    auto max_v = 0.0;

    for (auto &elem : grid.fluid_cells()) {

        int i = elem->i();
        int j = elem->j();

        if (std::fabs(_U(i, j)) > max_u) max_u = std::fabs(_U(i, j));

        if (std::fabs(_V(i, j)) > max_v) max_v = std::fabs(_V(i, j));
    }

    auto factor1 = 1 / (1 / (grid.dx() * grid.dx()) + 1 / (grid.dy() * grid.dy()));
    factor1 = factor1 / (2 * _nu);
    auto factor2 = grid.dx() / max_u;
    auto factor3 = grid.dy() / max_v;
    _dt = _tau * std::min(factor1, std::min(factor2, factor3));
    return _dt;
}

double Fields::calculate_dt_e(Grid &grid) {

    auto max_u = 0.0;
    auto max_v = 0.0;

    for (auto &elem : grid.fluid_cells()) {

        int i = elem->i();
        int j = elem->j();

        if (std::fabs(_U(i, j)) > max_u) max_u = std::fabs(_U(i, j));

        if (std::fabs(_V(i, j)) > max_v) max_v = std::fabs(_V(i, j));
    }

    auto factor = 1 / (1 / (grid.dx() * grid.dx()) + 1 / (grid.dy() * grid.dy()));
    auto factor1 = factor / (2 * _nu);
    auto factor2 = grid.dx() / max_u;
    auto factor3 = grid.dy() / max_v;
    auto factor4 = factor / (2 * _alpha);
    _dt = _tau * std::min(std::min(factor1, std::min(factor2, factor3)), factor4);
    return _dt;
}

double &Fields::p(int i, int j) { return _P(i, j); }
double &Fields::u(int i, int j) { return _U(i, j); }
double &Fields::v(int i, int j) { return _V(i, j); }
double &Fields::t(int i, int j) { return _T(i, j); }
double &Fields::f(int i, int j) { return _F(i, j); }
double &Fields::g(int i, int j) { return _G(i, j); }
double &Fields::rs(int i, int j) { return _RS(i, j); }

Matrix<double> &Fields::p_matrix() { return _P; }

double Fields::dt() const { return _dt; }
