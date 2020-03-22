## 컴파일러 제작

### 컴파일러의 필요성

- 인간의 문제를 해결하기 위해 컴퓨터를 사용함.
- 컴퓨터와 의사소통을 하기 위해 언어가 필요함.
  인간이 기계어를 사용하여 문제를 표현하기란 어렵기 때문에 사람 중심 언어인 고급 언어를 사용.
  인간이 사용하는 고급언어는 컴퓨터가 이해하지 못함.
  따라서 인간이 사용하는 **고급 언어를 기계어로 변환해주는 번역기**인 컴파일러가 필요함.

### 컴파일러의 일반적 구조

![컴파일러 구조](https://user-images.githubusercontent.com/59176149/77255665-7fa64800-6cac-11ea-9428-e7b8bb1fb067.png)

컴파일러의 구조는 크게 **전단부(front-end)** 와 **후단부(back-end)** 로 나눌 수 있다.

**전단부(front-end)** 는 소스 언어에 관계되는 부분으로 소스프로그램을 분석하고 중간 코드를 생성하는 부분 즉, 아래 단계에서 1.어휘 분석(lexical analysis) ~ 4.중간 코드의 생성(intermediate representation)단계.

**후단부(back-end)** 는 소스 언어 보다는 목적 기계(target machine)에 의존적이며 전단부에서 생성한 중간 코드를 특정 기계를 위한 목적 코드로 변환하는 부분 전반부 이후 나머지부분.


### 컴파일 단계
1. 어휘 분석(lexical analysis) or 스캔(scan)

  모듈: 어휘 분석기(lexical analyzer) or 스캐너(scanner)
  
  내용: 문자열을 의미있는 토큰(token)으로 변환
  
  결과물: 토큰(token)
  
2. 구분 분석(syntax analyzing)

  모듈: 파서(parser) or 구문 분석기(syntax analyzer)
  
  내용 : 토큰(token)을 구조를 가진 구문트리(syntax tree)로 변환
  
  결과물 : 구문 트리(syntax tree), parser 에 따라 추상 구문 트리(abstract syntax tree)를 바로 생성하기도 함
  
  ※ 구문트리(syntax tree) 와 추상구문트리(abstract syntax tree)의 차이는, 구문트리에는 세미콜론(;)이나 괄호 등이 모두 포함된다. 하지만 추상구문트리에는 이러한 불필요한(의미가 없는) 것들이 생략된다.
  
  
3. 의미 분석(semantic analysis)

  모듈 : 의미 분석기(semantic analyzer)
  
  내용 : 프로그램의 의미(semantic)에 따라 필요한 정보를 유추/분석
  
  결과물 : 추상 구문 트리(abstract syntax tree) / 장식구문(decoration syntax tree)


4. 중간 코드의 생성(intermediate representation)

  내용 : 좀 더 코드 생성이 편하고, 여러 종류의 언어나 기계어(CPU 종속)에 대응하기 위해서, 중간에 공통의 중간표현으로 변환


5. 코드 생성(code generation)

  모듈 : 코드 제너레이터(code generator)
  
  내용 : 어셈블리어나 기계가 이해하기 쉬운 명령으로 변환
  
  결과물 : 어셈블리어

6. 최적화(optimization)

  내용 : 좀더 질 좋은 프로그램으로 변환
  
  
7. 어셈블러

  내용 : 기계어로 변환
  
  결과물 : 기계어
  
