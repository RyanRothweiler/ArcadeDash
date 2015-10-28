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
		foreach (Enemy enemy in enemies)
		{
			enemiesLeft++;
		}
		enemiesLeftText.text = "" + enemiesLeft;
	}
}
