//
// Created by cds on 2020/12/15.
//

#include <string>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <iostream>

using namespace std;


//---------------状态机架构开始----------------------
typedef int eState;
eState NullState = -1;

class FSM;

// 状态的基类
class State
{
 public:
  virtual void OnStateEnter() = 0;
  virtual void OnStateTick() = 0;
  virtual void OnStateExit() = 0;

  State(FSM* _fsm)
  {
    fsm = _fsm;
  }
 protected:
  FSM* fsm = nullptr;
};

// 状态机，统管所有状态
class FSM
{
 private:
  unordered_map<eState, State*> all_states;
  eState curState = -1;

 public:
  // 注册新的状态
  bool Register(eState e, State* pState)
  {
    all_states[e] = pState;
    return true;
  }
  // 需要状态转移则调用此函数
  bool Trans(eState newState)
  {
    all_states[curState]->OnStateExit();

    all_states[newState]->OnStateEnter();
    curState = newState;
    return true;
  }
  // 设置初始状态
  void SetInitState(eState s)
  {
    curState = s;
  }
  // 每帧调用
  void Tick()
  {
    all_states[curState]->OnStateTick();
  }
};

// -----------------实际使用例子开始-----------------
const int State_Idle = 1;
const int State_Attack = 2;
const int State_GoHome = 3;

// 实际应用，有三个具体的状态，都从State继承，然后分别实现自己的Enter、Tick、Exit函数

class StateIdle : public State
{
 public:
  StateIdle(FSM* fsm) : State(fsm) {}

  int test_counter = 0;

  void OnStateEnter() {
    cout << "---- Idle Enter" << endl;
    test_counter = 0;
  }

  void OnStateTick() {
    cout << "Idle Tick" << endl;
    // 模拟一个定时状态转移。在实际工程中是根据需求逻辑写的
    test_counter++;
    if (test_counter == 5)
    {
      fsm->Trans(State_Attack);
    }
  }

  void OnStateExit() { cout << "==== Idle Exit" << endl; }
};

class StateAttack : public State
{
 public:
  StateAttack(FSM* fsm) : State(fsm) {}

  void OnStateEnter() { cout << "---- Attack Enter" << endl; }

  void OnStateTick() { cout << "Attack Tick" << endl; }

  void OnStateExit() { cout << "==== Attack Exit" << endl; }
};

class StateGoHome : public State
{
 public:
  StateGoHome(FSM* fsm) : State(fsm) {}

  void OnStateEnter() { cout << "---- GoHome Enter" << endl; }

  void OnStateTick() { cout << "GoHome Tick" << endl; }

  void OnStateExit() { cout << "==== GoHome Exit" << endl; }
};

int main()
{
  // 构造状态机
  FSM* fsm = new FSM();
  State* stateIdle = new StateIdle(fsm);
  State* stateAttack = new StateAttack(fsm);
  State* stateGoHome = new StateGoHome(fsm);

  fsm->Register(State_Idle, stateIdle);
  fsm->Register(State_Attack, stateAttack);
  fsm->Register(State_GoHome, stateGoHome);
  fsm->SetInitState(State_Idle);

  // 将状态机Tick放入程序主循环中, 仅示范
  while (true)
  {
    fsm->Tick();
    sleep(1);
  }

  return 0;
}