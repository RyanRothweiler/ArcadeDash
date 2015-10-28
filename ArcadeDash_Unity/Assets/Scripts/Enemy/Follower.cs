using UnityEngine;
using System.Collections;

public class Follower : MonoBehaviour
{

	// Use this for initialization
	void Start ()
	{

	}

	// Update is called once per frame
	void Update ()
	{
		if (Vector3.Distance(this.transform.position, PlayerController.instance.transform.position) < 4.5f)
		{
			Vector3 playerDir = (this.transform.position - PlayerController.instance.transform.position).normalized;

			float angle = Mathf.Atan2(playerDir.y, playerDir.x) * Mathf.Rad2Deg;
			this.transform.rotation = Quaternion.AngleAxis(angle, Vector3.forward);

			this.transform.position = this.transform.position - (playerDir * 0.035f);
		}
	}
}
