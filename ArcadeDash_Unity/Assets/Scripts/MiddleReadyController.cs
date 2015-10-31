using UnityEngine;
using System.Collections;
using UnityEngine.UI;


public class MiddleReadyController : MonoBehaviour
{

	public Text baddieCountText;

	public void Start()
	{
		baddieCountText.text = GlobalState.instance.enemyCount + " baddies in next level";
	}

	// Update is called once per frame
	void Update ()
	{
		if (Input.GetButtonDown("Stop"))
		{
			Application.LoadLevel("LevelGenTest");
		}
	}
}
