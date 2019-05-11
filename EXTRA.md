# Enhancing WRR Schduler

### 1. priority and aging

WRR scheduler는 preemption 없이 새로운 태스크가 반드시 run queue의 끝에 추가된다. 이는 태스크들 사이의 fairness를 보장할 수 있으나, 우선순위가 높은 태스크가 들어왔을 때에도 해당 태스크의 우선적인 실행이 보장되지 않는다는 면에서 비효율적일 수 있다. 따라서 이를 개선하기 위하여 제시하는 방법은 다음과 같다.

1. WRR 태스크는 weight와는 별개로 priority 값을 갖는다. (e.g. 0~5)
1. 또한 WRR 태스크는 각각 run queue에 자신 enqueue 되었을 때의 시간을 저장한다.
1. WRR scheduler가 하나가 아닌 priority 개수 만큼의 run queue를 가진다.
1. 새로운 WRR 태스크가 enqueue되면, 해당 태스크는 자신의 priority에 맞는 run queue에 enqueue된다.
1. scheduler는 다음에 실행할 태스크를 선택할 때, priority 순서대로 run queue를 살펴보면서 가장 높은 priority run queue의 맨 앞 태스크를 선택한다.
1. 주기적으로 각 run queue의 맨 앞 태스크를 살펴보고, enqueue된 시간이 일정 시간 이상 지났으면, 해당 태스크를 상위 run queue로 이동시킨다. (aging)

위와 같은 방식을 사용하면, 같은 WRR 태스크 사이에서의 우선순위를 두어 우선순위가 높은 태스크가 우선적으로 실행되게 할 수 있다. 또한, aging 방식을 사용하여 우선순위가 낮은 태스크도 starvation이 발생하지 않고 실행됨이 보장된다.

### 2. CPU selection on fork & group scheduling

WRR scheduler는 새로운 프로세스가 할당될 CPU를 정할 때 가장 weight sum이 작은 CPU를 선택한다. 그러나 같은 메모리를 공유하는 parent-child의 경우는 같은 CPU에 할당되는 것이 cache 측면에서 효율성을 높일 수 있다. 따라서 이를 개선하기 위해 제시하는 방법은 다음과 같다.

parent-child 프로세스를 하나의 스케쥴링 단위로 만들고, 새로운 child 프로세스가 fork되면 이를 parent와 같은 CPU에 할당한다. load balacing이 일어날 때는, 하나의 태스크만 골라 다른 CPU로 migration하는 대신, parent와 child를 동시에 옮겨서 cache 효율성을 높인다.