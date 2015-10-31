using UnityEngine;
using System.Collections;

public class GlobalState : MonoBehaviour
{

	public static GlobalState instance;

	public int enemyCount = 10;

	void Start ()
	{
		DontDestroyOnLoad(this.gameObject);
		instance = this;
	}

}