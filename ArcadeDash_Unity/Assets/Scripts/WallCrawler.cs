using UnityEngine;
using System.Collections;

public class WallCrawler : MonoBehaviour
{

	public GameObject topTarget;
	public GameObject bottomTarget;

	private GameObject currentTarget;


	public void Start()
	{
		currentTarget = topTarget;
	}

	void Update ()
	{
		this.transform.position = this.transform.position - ((this.transform.position - currentTarget.transform.position).normalized * 0.03f);
		// Debug.Log(Vector3.Distance(this.transform.position, currentTarget.transform.position));
		if (Vector3.Distance(this.transform.position, currentTarget.transform.position) < 0.2f)
		{
			if (currentTarget == topTarget)
			{
				currentTarget = bottomTarget;
			}
			else
			{
				currentTarget = topTarget;
			}
		}
	}
}
