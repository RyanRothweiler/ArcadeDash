using UnityEngine;
using System.Collections;
using UnityEngine.UI;


public class UIController : MonoBehaviour
{

	public Text enemiesLeftText;

	// Update is called once per frame
	void Update ()
	{
		int enemiesLeft = 0;
		Enemy[] enemies = FindObjectsOfType(typeof(Enemy)) as Enemy[];
		for (int index = 0;
		     index < enemies.Length;
		     index++)
		{
			enemiesLeft++;
		}
		enemiesLeftText.text = "" + enemiesLeft;
	}
}
