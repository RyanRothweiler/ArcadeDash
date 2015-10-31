using UnityEngine;
using System.Collections;

public class DeadSceneController : MonoBehaviour
{
	// Update is called once per frame
	void Update ()
	{
		if (Input.GetButtonDown("Stop"))
		{
			GlobalState.instance.enemyCount = 8;
			Application.LoadLevel("LevelGenTest");
		}
	}
}
