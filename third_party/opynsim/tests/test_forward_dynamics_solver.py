import opynsim as opyn
import opynsim.examples

def test_forward_dynamics_solver_works_on_basic_example():
    import numpy as np

    samples = 5
    step_size = 1/(samples-1)

    model = opyn.examples.pendulum_model()
    state = model.initial_state()
    solver = opyn.ForwardDynamicsSolver(model, state)
    emitted_states = []
    for t in np.linspace(0, 1, samples):
        emitted_states.append(solver.integrate_to(t, realized_to=opyn.STAGE_ACCELERATION))

    for i in range(0, samples):
        assert emitted_states[i].time  == (i * step_size)
        assert emitted_states[i].stage >= opyn.STAGE_ACCELERATION

def test_forward_dynamics_solver_model_state_returns_supplied_state():
    model = opyn.examples.pendulum_model()
    advanced_state = opyn.ForwardDynamicsSolver(model, model.initial_state()).integrate_to(0.1)

    assert opyn.ForwardDynamicsSolver(model, advanced_state).model_state == advanced_state

def test_forward_dynamics_solver_can_be_constructed_without_an_initial_state():
    model = opyn.examples.pendulum_model()
    solver = opyn.ForwardDynamicsSolver(model)

    assert solver.model_state == model.initial_state()