#include <iostream>
#include <vector>



bool isPalindrom(std::vector<int> v, int target)
{

	int left = 0;
	int right = v.size() - 1;

	while(left < right)
	{

		int sum = v[left] + v[right];

		if(sum == target)
		{
			return true;
		}else if(sum < target){
			left++;
		}else{
			right--;
		}
	}

	return false;
}


int main()
{

	std::vector<int> v{1,4,3,4,5};
	int target = 8;

	if(isPalindrom(v, target))
	{
		std::cout << "true"; 
	}else{
		std::cout << "false";
	}

	
}
