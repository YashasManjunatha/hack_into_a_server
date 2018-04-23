package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {

	Timer electionCountTimer;
	int electionTimeout;
	final int electionTimerID = 2;

	public void go () {
		synchronized (mLock) {
			log("switched to candidate mode.");
			electionStart(); // This will invoke the requestVoteRPC of all other servers
		}
	}
	
	public void electionStart() {
		int term = mConfig.getCurrentTerm() + 1; // Increment term to indicate start of new election
		mConfig.setCurrentTerm(term, mID);
		
		log("starting new election.");
		
		RaftResponses.setTerm(term);
		RaftResponses.clearVotes(term);
		
		for (int i = 0; i < mConfig.getNumServers(); i++) {
			if (i != mID) {
				this.remoteRequestVote(i, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
			}
		}
		
		if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
			electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}

		electionCountTimer = scheduleTimer(electionTimeout, electionTimerID);
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

			// If in CandidateMode, already voted for self so return term
			// likely a bit more sophisticated than this. 
			// return self.term

			int term = mConfig.getCurrentTerm ();
			int result = term;
			return result;
		}
	}


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

			// Two cases: 1. Stale leader, 2. Stale candidate


			int term = mConfig.getCurrentTerm ();
			int result = term;
			return result;
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				log("handling timeout");
				int[] votes = new int[mConfig.getNumServers()];
				for (int i : RaftResponses.getVotes(mConfig.getCurrentTerm())) {
					votes[i]++;
				}
				
				boolean lostElection = false;
				for (int id = 0; id < votes.length; id++) {
					if (id != mID && votes[mID] > Math.ceil(mConfig.getNumServers()/2.0)) {
						log(" another leader won! P"+mID+":"+mConfig.getCurrentTerm());
						lostElection = true;
					}
				}

				if (votes[mID] > Math.ceil(mConfig.getNumServers()/2.0)) { // 1. Won election (become leader)
					log("won election");
					RaftServerImpl.setMode(new LeaderMode());
				} else if (lostElection) { // 2. Lost election (become follower)
					log("lost election");
					RaftServerImpl.setMode(new FollowerMode());
				} else { // 3. Still waiting on vote (reset timeout) // new election?
					electionCountTimer.cancel();
					electionStart();
				}
			}
		}
	}

	public void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + ": " + message);
	}
}
