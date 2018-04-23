package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

// notes:
// Respond to RPCs from candidates and leaders
// If election timeout elapses without receiving AppendEntries
// RPC from current leader or granting vote to candidate:
// convert to candidate

/* 5.2
 * A server remains in follower state as long as it receives valid
 * RPCs from a leader or candidate. Leaders send periodic
 * heart beats (AppendEntries RPCs that carry no log entries)
 * to all followers in order to maintain their authority. If a
 * follower receives no communication over a period of time
 * called the election timeout, then it assumes there is no viable
 * leader and begins an election to choose a new leader.
 */

public class FollowerMode extends RaftMode {
	
	Timer electionTimer; // if the electionTimer times out, become a candidate
	final int electionTimerID = 1;

	public void go () {
		synchronized (mLock) {
			int currentTerm = mConfig.getCurrentTerm();
			
			log(currentTerm, "switched to follower mode.");
			
			// start timer
			int electionTimeout; 
			if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
				electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
			}

			electionTimer = scheduleTimer(electionTimeout, electionTimerID);
		}
	}

	// @param candidate’s term
	// @param candidate requesting vote
	// @param index of candidate’s last log entry
	// @param term of candidate’s last log entry
	// @return 0, if server votes for candidate; otherwise, server's
	// current term
	public int requestVote (int candidateTerm,
			int candidateID,
			int lastLogIndex,
			int lastLogTerm) {
		synchronized (mLock) {
			int currentTerm = mConfig.getCurrentTerm ();
			
			/* Reply false if term < currentTerm (§5.1)
			 * If votedFor is null or candidateId, and candidate’s log is at least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
			 * 
			 * Raft determines which of two logs is more up-to-date
			 * by comparing the index and term of the last entries in the
			 * logs. If the logs have last entries with different terms, then
			 * the log with the later term is more up-to-date. If the logs
			 * end with the same term, then whichever log is longer is
			 * more up-to-date. 
			 */
			
			boolean candidateOutdatedTerm = candidateTerm < currentTerm;
			boolean haventVoted = (mConfig.getVotedFor() == candidateID || mConfig.getVotedFor() == 0);
			boolean candidateLogUpToDate = (lastLogTerm == mLog.getLastTerm()) ? lastLogIndex > mLog.getLastIndex() : lastLogTerm > mLog.getLastTerm();
			
			if (!candidateOutdatedTerm || haventVoted || candidateLogUpToDate) {
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				return 0; // Vote for server, return 0
			} else {
				//mConfig.setCurrentTerm(currentTerm, 0);
				return currentTerm; // No vote, return term
			}
		}
	}
	
	/* 5.2
	 * While waiting for votes, a candidate may receive an
	 * AppendEntries RPC from another server claiming to be
	 * leader. If the leader’s term (included in its RPC) is at least
	 * as large as the candidate’s current term, then the candidate
	 * recognizes the leader as legitimate and returns to follower
	 * state. If the term in the RPC is smaller than the candidate’s
	 * current term, then the candidate rejects the RPC and continues
	 * in candidate state.
	 */

	
	/*
	 * Receiver implementation:
	 * 1. Reply false if term < currentTerm (§5.1)
 	 * 2. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm (§5.3)
     * 3. If an existing entry conflicts with a new one (same index but different terms), delete the existing entry and all that follow it (§5.3)
	 * 4. Append any new entries not already in the log
	 * 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
	 */
	
	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID,
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {
			
			
			boolean legitimateLeader = leaderTerm >= mConfig.getCurrentTerm();

			if (legitimateLeader) {
				// reset timer, attempt to repair log.
			} else {
				// reject RPC.
			}
			
			// 1. Check if appendEntriesRPC came from leader
			// 2. If so, reset timer, and attempt to repair log

			return 0;
		}
	}  

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				// If timer goes off, hold election
				electionTimer.cancel();
				RaftServerImpl.setMode(new CandidateMode());
			}
		}
	}

	public void log(int term, String message) {
		System.out.println ("S" + mID + "." + term + ": " + message);
	}
}

