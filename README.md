# pintos

## Project 1 : Threads (9.10 ~ 9.23)

- busy waiting
  - 처음 implementation은 busy waiting 이다. timer_sleep 이 while문으로 시간을 매번 체크해서 조건에 부합할 때, thread_yield()를 하는 것을 알 수 있다. 이러한 연산은 CPU의 연산을 잡아먹어, 자원을 낭비하게 된다. busy waiting이 아닌 형태로 바꾼다.
  - 방법 중 하나는 sleep_list를 만든다. sleep 해야하는 thread를 깨워져야하는 시간을 기록하여 sleep_list에 보관한 후, timer_interrupt에서 지속적으로 확인하여 깨워준다. 
- Priority Scheduling

