using UnityEngine;
using System.Collections;

public class GlobalState : MonoBehaviour
{

	public static GlobalState instance;

	public int enemyCount;

	void Start ()
	{
		// GlobalState[] objs = FindObjectsOfType(typeof(GlobalState)) as GlobalState[];
		if (instance != null &&
		    instance != this)
		{
			Destroy(this.gameObject);
		}
		else
		{
			DontDestroyOnLoad(this.gameObject);
			instance = this;
		}
	}

}