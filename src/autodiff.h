#pragma once

#include<queue>
#include<set>
#include <memory>

using namespace std;



namespace autodiff {
	enum OpType
	{
		PLACEHOLDER,	//0
		CONST_VAL,			
		EQUAL,			
		ADD,			
		SUB,			//4
		MUL,
		DIV,
		SIN,			//7
		COS,
		EXP,
		LOGE,			//10
		LOG10,
		LOG2,
		SQRT,
		NEG
	};

	/*
	�Զ�΢���еı�����Ҳ���Զ�΢�ּ���ͼ�еĽڵ�
	*/
	template<typename T>
	struct ADV_Data {
	public:
		OpType op;			//�����ñ��������
		shared_ptr<ADV_Data<T>> var[2];		//��������ұ���
		T val;	//�ñ�����ֵ
		T dval; //ĳ������Ըñ�����ƫ����ֵ
		static int cnt;
		int id;

		ADV_Data() {
			val = 0;
			dval = 0;
		}
	};
	template<typename T> int ADV_Data<T>::cnt = 0;

	//================================================================================================

	//================================================================================================
	template<typename T>
	class ADV {
	public:
		shared_ptr<ADV_Data<T>> ADVptr;
		ADV() {					//�ڡ�ADV x"������µ���
			ADVptr = shared_ptr<ADV_Data<T>>(new ADV_Data<T>);
			ADVptr->op = PLACEHOLDER;
			ADVptr->var[0] = NULL;	ADVptr->var[1] = NULL;
			ADVptr->id = ADVptr->cnt; ADVptr->cnt++;
		}
		ADV(shared_ptr<ADV_Data<T>> ptr)
		{
			ADVptr = ptr;
		}
		ADV(const ADV &adv)	//�������캯�����ڡ�ADV x=y", �Լ���z=x+y"������µ���.
		{
			//ADVptr = shared_ptr<ADV<T>>(new ADV<T>);
			//(*this)().op = EQUAL;
			//(*this)().optrR = adv.ADVptr;
			//(*this)().val = adv()->val;
			ADVptr = adv.ADVptr;
		}
		ADV(const T val) {		//�ڡ�ADV x=8.8��������µ���
			ADVptr = shared_ptr<ADV_Data<T>>(new ADV_Data<T>);
			ADVptr->id = ADVptr->cnt; ADVptr->cnt++;
			ADVptr->val = val;
			ADVptr->op = PLACEHOLDER;
			ADVptr->var[0] = NULL;	ADVptr->var[1] = NULL;
		}
		ADV<T>& operator=(const ADV<T> &rhs)	//�ȺŸ�ֵ�� ��z=x+y"ʱ��ͨ���������캯������һ����ʱADV����ͨ���ȺŸ�ֵ��z
		{
			if (this == &rhs)
				return *this;
			else {
				ADVptr = rhs.ADVptr;
				return *this;
			}
		}
		ADV<T>& operator=(const T &val)			//�ȺŸ�ֵ�� ��z=1.8"ʱ��ͨ���������캯������һ����ʱADV����ͨ���ȺŸ�ֵ��z
		{
			ADVptr->val = val;
			return *this;
		}

		ADV_Data<T>*  operator()() const {
			return ADVptr.get();
		}
		//shared_ptr<ADV_Data<T>> get() const { return ADVptr; }
	}; //end of class

	template<typename T>
	ostream& operator<<(ostream &os, const ADV<T> &adv) {
		os << adv()->val;
		return os;
	}

	template<typename T>
	ADV<T> operator+(const ADV<T> &x, const ADV<T> &y) 	//�ӷ���˫Ŀ�����
	{
		ADV<T> adv;
		adv()->val = y()->val + x()->val;
		adv()->op = ADD;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator+(const ADV<T> &x, const T &y) 	//�ӷ���˫Ŀ�����
	{
		ADV<T> adv, adv_y;
		adv_y()->op = CONST_VAL; adv_y()->val = y;
		adv()->val = x()->val + y;
		adv()->op = ADD;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = adv_y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator+(const T &x, const ADV<T> &y) 	//�ӷ���˫Ŀ�����
	{
		ADV<T> adv, adv_x;
		adv_x()->op = CONST_VAL; adv_x()->val = x;
		adv()->val = x+ y()->val;
		adv()->op = ADD;
		adv()->var[0] = adv_x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator-(const ADV<T> &x) 	//�ӷ���˫Ŀ�����
	{
		ADV<T> adv;
		adv()->val = -x()->val;
		adv()->op = NEG;
		adv()->var[0] = x.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator-(const ADV<T> &x, const ADV<T> &y) 	//������ADV+ADV, ˫Ŀ�����
	{
		ADV<T> adv;
		adv()->val = x()->val - y()->val;
		adv()->op = SUB;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}
	template<typename T>
	ADV<T> operator-(const ADV<T> &x, const T &y) 	//������ADV-scalar
	{
		ADV<T> adv, adv_y;
		adv_y()->op = CONST_VAL; adv_y()->val = y;
		adv()->val = x()->val - y;
		adv()->op = SUB;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = adv_y.ADVptr;
		return adv;
	}
	template<typename T>
	ADV<T> operator-(const T &x, const ADV<T> &y) 	//������scalar+ADV, ˫Ŀ�����
	{
		ADV<T> adv, adv_x;
		adv_x()->op = CONST_VAL; adv_x()->val = x;
		adv()->val =x- y()->val;
		adv()->op = SUB;
		adv()->var[0] = adv_x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator*(const ADV<T> &x, const ADV<T> &y) 	//�˷���ADV*ADV, ˫Ŀ�����
	{
		ADV<T> adv;
		adv()->val = x()->val * y()->val;
		adv()->op = MUL;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator*(const ADV<T> &x, const T &y) 	//�˷���ADV*scalar, ˫Ŀ�����
	{
		ADV<T> adv,adv_y;		
		adv_y()->op = CONST_VAL; adv_y()->val = y;
		adv()->val = x()->val * y;
		adv()->op = MUL;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = adv_y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator*(const T &x, const ADV<T> &y) 	//�˷���scalar*ADV, ˫Ŀ�����
	{
		ADV<T> adv, adv_x;
		adv_x()->op = CONST_VAL; adv_x()->val = x;
		adv()->val = x * y()->val;
		adv()->op = MUL;
		adv()->var[0] = adv_x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator/(const ADV<T> &x, const ADV<T> &y) 	//������ADV/ADV, ˫Ŀ�����
	{
		ADV<T> adv;
		adv()->val = x()->val / y()->val;
		adv()->op = DIV;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator/(const ADV<T> &x, const T &y) 	//������ADV/scalar, ˫Ŀ�����
	{
		ADV<T> adv,adv_y;
		adv_y()->op = CONST_VAL; adv_y()->val = y;
		adv()->val = x()->val / y;
		adv()->op = DIV;
		adv()->var[0] = x.ADVptr;	adv()->var[1] = adv_y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> operator/(const T &x, const ADV<T> &y) 	//������scalar/ADV, ˫Ŀ�����
	{
		ADV<T> adv, adv_x;
		adv_x()->op = CONST_VAL; adv_x()->val = x;
		adv()->val = x / y()->val;
		adv()->op = DIV;
		adv()->var[0] = adv_x.ADVptr;	adv()->var[1] = y.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> sin(ADV<T> &x) 	//����
	{
		ADV<T> adv;
		adv()->val = std::sin(x()->val);
		adv()->op = SIN;
		adv()->var[0] = x.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> cos(ADV<T> &x) 	//����
	{
		ADV<T> adv;
		adv()->val = std::cos(x()->val);
		adv()->op = COS;
		adv()->var[0] = x.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> exp(ADV<T> &x) 	//����
	{
		ADV<T> adv;
		adv()->val = std::exp(x()->val);
		adv()->op = EXP;
		adv()->var[0] = x.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> ln(ADV<T> &x) 	//��Ȼ����
	{
		ADV<T> adv;
		adv()->val = std::log(x()->val);
		adv()->op = LOGE;
		adv()->var[0] = x.ADVptr;
		return adv;
	}

	template<typename T>
	ADV<T> sqrt(ADV<T> &x) 	//����
	{
		ADV<T> adv;
		adv()->val = std::sqrt(x()->val);
		adv()->op = SQRT;
		adv()->var[0] = x.ADVptr;
		return adv;
	}


	template<typename T>
	ADV<T> dot(ADV<T> *a, ADV<T> *b)
	{
		ADV<T> result;
		result = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
		return result;
	}

	/*
	��ά�����Ĳ��
	*/
	template<typename T>
	void cross(ADV<T> *a, ADV<T> *b, ADV<T> *out)
	{
		out[0] = a[1] * b[2] - a[2] * b[1];
		out[1] = a[2] * b[0] - a[0] * b[2];
		out[2] = a[0] * b[1] - a[1] * b[0];
	}


#define DVAR ADV<double>

}