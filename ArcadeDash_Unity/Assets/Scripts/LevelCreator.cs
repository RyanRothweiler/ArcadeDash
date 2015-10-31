using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class LevelCreator : MonoBehaviour
{

	public List<GameObject> enemyFabs;
	public GameObject wallFab;


	//TODO maybe remove this
	private Vector2 levelBounds = new Vector2(15, 8);

	void Start ()
	{
		GenerateLevel(GlobalState.instance.enemyCount);
	}

	public void GenerateLevel(int enemyCount)
	{

		// wall generation
		int wallCount = Random.Range(8, 13);
		for (int index = 0;
		     index < wallCount;
		     index++)
		{
			GameObject newWall = Instantiate(wallFab) as GameObject;

			Vector3 newPos = new Vector3(Random.Range(-levelBounds.x, levelBounds.x),
			                             Random.Range(-levelBounds.y, levelBounds.y),
			                             0);
			newWall.transform.position = newPos;


			Vector2 scaleDir = Vector2.zero;
			if (Random.Range(0, 2) == 1)
			{
				scaleDir = new Vector2(1, 0);
			}
			else
			{
				scaleDir = new Vector2(0, 1);
			}
			float randomScale = Random.Range(0, 5);
			Vector3 newScale = new Vector3(0.5f + (randomScale * scaleDir.x), 0.5f + (randomScale * scaleDir.y), 1);
			newWall.transform.localScale = newScale;
		}


		Wall[] walls = FindObjectsOfType(typeof(Wall)) as Wall[];

		// enemy generation
		int enemiesMade = 0;
		while (enemiesMade < enemyCount)
		{
			GameObject enemyFab = enemyFabs[Random.Range(0, enemyFabs.Count)];
			GameObject newEnemy = Instantiate(enemyFab);

			Vector3 newPos = new Vector3(Random.Range(-levelBounds.x, levelBounds.x),
			                             Random.Range(-levelBounds.y, levelBounds.y),
			                             0);
			newEnemy.transform.position = newPos;

			bool enemyValid = true;
			foreach (Wall wall in walls)
			{
				Bounds enemyBounds = newEnemy.GetComponent<Collider2D>().bounds;
				if (wall.GetComponent<Collider2D>().bounds.Intersects(enemyBounds))
				{
					enemyValid = false;
					Destroy(newEnemy);
				}
			}

			if (enemyValid)
			{
				enemiesMade++;

				// this is custom enemy behavior
				Enemy enemy = newEnemy.GetComponent<Enemy>();
				switch (enemy.type)
				{
					case EnemyType.WallCrawler:
					{

						GameObject baseWall = walls[Random.Range(0, walls.Length)].gameObject;


						bool verticalWall;
						if (baseWall.transform.localScale.x > baseWall.transform.localScale.y)
						{
							verticalWall = true;
						}
						else
						{
							verticalWall = false;
						}

						if (verticalWall)
						{
							newEnemy.transform.position = baseWall.transform.position - new Vector3(1, 0, 0);
							WallCrawler crawler = enemy.GetComponent<WallCrawler>();

							crawler.topTarget.transform.position = crawler.transform.position +
							                                       new Vector3(0, baseWall.transform.localScale.x, 0);
							crawler.bottomTarget.transform.position = crawler.transform.position -
							        new Vector3(0, baseWall.transform.localScale.x, 0);
						}
						else
						{
							newEnemy.transform.position = baseWall.transform.position - new Vector3(0, 1, 0);
							WallCrawler crawler = enemy.GetComponent<WallCrawler>();
							crawler.transform.Rotate(0, 0, -90);

							crawler.topTarget.transform.position = crawler.transform.position +
							                                       new Vector3(baseWall.transform.localScale.y, 0, 0);
							crawler.bottomTarget.transform.position = crawler.transform.position -
							        new Vector3(baseWall.transform.localScale.y, 0, 0);
						}


						break;
					}
				}
			}
		}
	}
}